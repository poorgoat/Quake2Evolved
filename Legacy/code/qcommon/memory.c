/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "qcommon.h"


/*
 =======================================================================

 ZONE MEMORY ALLOCATION

 There is never any space between blocks, and there will never be two
 contiguous free blocks.

 The rover can be left pointing at a non-empty block.

 The zone calls are pretty much only used for small strings and 
 structures, all big things are allocated on the hunk.
 =======================================================================
*/

#define MIN_ZONE_MEGS	16
#define MIN_ZONE_SIZE	16 << 20

#define SMALLZONE_SIZE	0x400000

#define ZONEID			0x1d4a11
#define MINFRAGMENT		64

typedef struct memBlock_s {
	struct		memBlock_s *next, *prev;
	int			size;				// Including the header and possibly tiny fragments
	int			tag;				// A tag of 0 is a free block
	int			id;					// Should be ZONEID
	struct		memZone_s *zone;	// Zone containing this block
} memBlock_t;

typedef struct memZone_s {
	int			size;				// Total bytes malloced, including header
	int			bytes;				// Total bytes in use
	int			blocks;				// Total blocks in use
	memBlock_t	blockList;			// Start/end cap for linked list
	memBlock_t	*rover;
} memZone_t;

static memZone_t	*smallZone;
static memZone_t	*mainZone;


/*
 =================
 Z_ClearZone
 =================
*/
void Z_ClearZone (memZone_t *zone, int size){

	memBlock_t	*block;

	// Set the entire zone to one free block
	zone->size = size;
	zone->bytes = 0;
	zone->blocks = 0;
	zone->blockList.next = zone->blockList.prev = block = (memBlock_t *)((byte *)zone + sizeof(memZone_t));
	zone->blockList.size = 0;
	zone->blockList.tag = 1;	// In use block
	zone->blockList.id = 0;
	zone->blockList.zone = zone;
	zone->rover = block;

	block->prev = block->next = &zone->blockList;
	block->size = size - sizeof(memZone_t);
	block->tag = 0;				// Free block
	block->id = ZONEID;
	block->zone = zone;
}

/*
 =================
 Z_CheckHeap
 =================
*/
void Z_CheckHeap (memZone_t *zone){

	memBlock_t	*block;

	for (block = zone->blockList.next; block->next != &zone->blockList; block = block->next){
		if (*(int *)((byte *)block + block->size - sizeof(int)) != ZONEID)
			Com_Error(ERR_FATAL, "Z_CheckHeap: trashed memory block");
		if ((byte *)block + block->size != (byte *)block->next)
			Com_Error(ERR_FATAL, "Z_CheckHeap: block size does not touch the next block");
		if (block->next->prev != block)
			Com_Error(ERR_FATAL, "Z_CheckHeap: next block does not have proper back link");
		if (!block->tag && !block->next->tag)
			Com_Error(ERR_FATAL, "Z_CheckHeap: two consecutive free blocks");
		if (block->zone != zone)
			Com_Error(ERR_FATAL, "Z_CheckHeap: block zone links to invalid zone");
	}
}

/*
 =================
 Z_Malloc
 =================
*/
void *Z_Malloc (int size){

	return Z_TagMalloc(size, 1);
}

/*
 =================
 Z_MallocSmall
 =================
*/
void *Z_MallocSmall (int size){

	return Z_TagMalloc(size, -1);
}

/*
 =================
 Z_TagMalloc
 =================
*/
void *Z_TagMalloc (int size, int tag){

	memZone_t	*zone;
	memBlock_t	*start, *rover, *block, *fragment;
	void		*ptr;
	int			extra;

	// If main zone is not initialized or tag is -1 use the small zone
	if (mainZone == NULL || tag == -1)
		zone = smallZone;
	else
		zone = mainZone;

	// Debug tool for memory integrity checking
	if (com_debugMemory && com_debugMemory->integer)
		Z_CheckHeap(zone);

	if (size < 0)
		Com_Error(ERR_FATAL, "Z_TagMalloc: size < 0");

	if (!tag)
		Com_Error(ERR_FATAL, "Z_TagMalloc: tried to use a 0 tag");

	size += sizeof(memBlock_t);		// Account for size of block header
	size += sizeof(int);			// Space for memory trash tester
	size = (size + 7) & ~7;			// Align to 8-byte boundary

	// Scan through the block list looking for the first free block of
	// sufficient size
	block = rover = zone->rover;
	start = block->prev;

	do {
		if (rover == start){
			// Scanned all the way around the list
			if (zone == smallZone)
				Com_Error(ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the small zone", size);
			else
				Com_Error(ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the main zone", size);
		}

		if (rover->tag)
			block = rover = rover->next;
		else
			rover = rover->next;
	} while (block->tag != 0 || block->size < size);

	// Found a block big enough
	extra = block->size - size;
	if (extra > MINFRAGMENT){
		// There will be a free fragment after the allocated block
		fragment = (memBlock_t *)((byte *)block + size);
		fragment->size = extra;
		fragment->tag = 0;
		fragment->id = ZONEID;
		fragment->zone = zone;
		fragment->prev = block;
		fragment->next = block->next;
		fragment->next->prev = fragment;

		block->next = fragment;
		block->size = size;
	}

	// Increment counters
	zone->bytes += block->size;
	zone->blocks++;

	// Next allocation will start looking here
	zone->rover = block->next;

	block->tag = tag;		// No longer a free block
	block->id = ZONEID;
	block->zone = zone;

	// Marker for memory trash testing
	*(int *)((byte *)block + block->size - sizeof(int)) = ZONEID;

	// Return 0 filled memory
	ptr = (void *)((byte *)block + sizeof(memBlock_t));
	memset(ptr, 0, block->size - sizeof(memBlock_t) - sizeof(int));

	return ptr;
}

/*
 =================
 Z_Free
 =================
*/
void Z_Free (void *ptr){

	memZone_t	*zone;
	memBlock_t	*block, *other;

	if (!ptr)
		Com_Error(ERR_FATAL, "Z_Free: NULL pointer");

	block = (memBlock_t *)((byte *)ptr - sizeof(memBlock_t));

	// Memory trash test
	if (*(int *)((byte *)block + block->size - sizeof(int)) != ZONEID)
		Com_Error(ERR_FATAL, "Z_Free: trashed memory block");

	if (block->zone != smallZone && block->zone != mainZone)
		Com_Error(ERR_FATAL, "Z_Free: freed a pointer with invalid zone");
	if (block->id != ZONEID)
		Com_Error(ERR_FATAL, "Z_Free: freed a pointer without ZONEID");
	if (!block->tag)
		Com_Error(ERR_FATAL, "Z_Free: freed a freed pointer");

	zone = block->zone;

	// Decrement counters
	zone->bytes -= block->size;
	zone->blocks--;

	block->tag = 0;			// Mark as free

	other = block->prev;
	if (!other->tag){
		// Merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		
		if (block == zone->rover)
			zone->rover = other;

		block = other;
	}

	other = block->next;
	if (!other->tag){
		// Merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;

		if (other == zone->rover)
			zone->rover = block;
	}
}

/*
 =================
 Z_FreeTags
 =================
*/
void Z_FreeTags (int tag){

	memBlock_t	*block, *next;

	for (block = smallZone->blockList.next; block->next != &smallZone->blockList; block = next){
		// Grab next now, so if the block is freed we still have it
		next = block->next;

		if (block->tag == tag)
			Z_Free((void *)((byte *)block + sizeof(memBlock_t)));
	}

	for (block = mainZone->blockList.next; block->next != &mainZone->blockList; block = next){
		// Grab next now, so if the block is freed we still have it
		next = block->next;

		if (block->tag == tag)
			Z_Free((void *)((byte *)block + sizeof(memBlock_t)));
	}
}


/*
 =======================================================================

 HUNK MEMORY ALLOCATION

 The hunk manages the entire memory block given to Quake. It must be
 contiguous. Memory can be allocated from either the low or high end in
 a stack fashion. The only way memory is released is by resetting one of
 the pointers.

 Hunk allocations are guaranteed to be 32 byte aligned.
 =======================================================================
*/

#define MIN_HUNK_MEGS	48
#define MIN_HUNK_SIZE	48 << 20

#define HUNK_SENTINEL	0x1df001ed

typedef struct {
	int		size;			// Including this header
	int		sentinel;		// Should be HUNK_SENTINEL
} hunk_t;

static byte	*hunk_base;
static int	hunk_size;

static int	hunk_lowUsed;
static int	hunk_highUsed;

static int	hunk_lowMark;
static int	hunk_highMark;


/*
 =================
 Hunk_Check

 Run consistency and sentinel trashing checks
 =================
*/
void Hunk_Check (void){

	hunk_t	*h;

	for (h = (hunk_t *)hunk_base; (byte *)h != hunk_base + hunk_lowUsed; h = (hunk_t *)((byte *)h + h->size)){
		if (h->size < sizeof(hunk_t) || (byte *)h + h->size - hunk_base > hunk_size)
			Com_Error(ERR_FATAL, "Hunk_Check: bad size");
		if (h->sentinel != HUNK_SENTINEL)
			Com_Error(ERR_FATAL, "Hunk_Check: trashed sentinel");
	}

	for (h = (hunk_t *)(hunk_base + hunk_size - hunk_highUsed); (byte *)h != hunk_base + hunk_size; h = (hunk_t *)((byte *)h + h->size)){
		if (h->size < sizeof(hunk_t) || (byte *)h + h->size - hunk_base > hunk_size)
			Com_Error(ERR_FATAL, "Hunk_Check: bad size");
		if (h->sentinel != HUNK_SENTINEL)
			Com_Error(ERR_FATAL, "Hunk_Check: trashed sentinel");
	}
}

/*
 =================
 Hunk_Alloc
 =================
*/
void *Hunk_Alloc (int size){

	hunk_t	*h;
	void	*ptr;

	// Debug tool for memory integrity checking
	if (com_debugMemory && com_debugMemory->integer)
		Hunk_Check();

	if (size < 0)
		Com_Error(ERR_FATAL, "Hunk_Alloc: size < 0");

	size += sizeof(hunk_t);			// Account for size of block header
	size = (size + 31) & ~31;		// Align to cache line

	if (hunk_size - hunk_lowUsed - hunk_highUsed < size)
		Com_Error(ERR_FATAL, "Hunk_Alloc: failed on allocation of %i bytes", size);

	h = (hunk_t *)(hunk_base + hunk_lowUsed);
	hunk_lowUsed += size;

	h->size = size;
	h->sentinel = HUNK_SENTINEL;

	// Return 0 filled memory
	ptr = (void *)((byte *)h + sizeof(hunk_t));
	memset(ptr, 0, h->size - sizeof(hunk_t));

	return ptr;
}

/*
 =================
 Hunk_HighAlloc
 =================
*/
void *Hunk_HighAlloc (int size){

	hunk_t	*h;
	void	*ptr;

	// Debug tool for memory integrity checking
	if (com_debugMemory && com_debugMemory->integer)
		Hunk_Check();

	if (size < 0)
		Com_Error(ERR_FATAL, "Hunk_HighAlloc: size < 0");

	size += sizeof(hunk_t);			// Account for size of block header
	size = (size + 31) & ~31;		// Align to cache line

	if (hunk_size - hunk_lowUsed - hunk_highUsed < size)
		Com_Error(ERR_FATAL, "Hunk_HighAlloc: failed on allocation of %i bytes", size);

	hunk_highUsed += size;
	h = (hunk_t *)(hunk_base + hunk_size - hunk_highUsed);

	h->size = size;
	h->sentinel = HUNK_SENTINEL;

	// Return 0 filled memory
	ptr = (void *)((byte *)h + sizeof(hunk_t));
	memset(ptr, 0, h->size - sizeof(hunk_t));

	return ptr;
}

/*
 =================
 Hunk_SetLowMark
 =================
*/
void Hunk_SetLowMark (void){

	hunk_lowMark = hunk_lowUsed;
}

/*
 =================
 Hunk_SetHighMark
 =================
*/
void Hunk_SetHighMark (void){

	hunk_highMark = hunk_highUsed;
}

/*
 =================
 Hunk_ClearToLowMark
 =================
*/
void Hunk_ClearToLowMark (void){

	hunk_lowUsed = hunk_lowMark;
}

/*
 =================
 Hunk_ClearToHighMark
 =================
*/
void Hunk_ClearToHighMark (void){

	hunk_highUsed = hunk_highMark;
}

/*
 =================
 Hunk_Clear
 =================
*/
void Hunk_Clear (void){

	hunk_lowUsed = 0;
	hunk_highUsed = 0;

	hunk_lowMark = 0;
	hunk_highMark = 0;

	Com_Printf("Hunk_Clear: reset the hunk, ok\n");
}


// =====================================================================


/*
 =================
 CopyString
 =================
*/
char *CopyString (const char *string){

	int		len;
	char	*buffer;

	if (!string)
		Com_Error(ERR_FATAL, "CopyString: NULL string\n");

	len = strlen(string);
	buffer = Z_MallocSmall(len+1);
	memcpy(buffer, string, len);

	return buffer;
}

/*
 =================
 FreeString
 =================
*/
void FreeString (char *string){

	if (!string)
		Com_Error(ERR_FATAL, "FreeString: NULL string\n");

	Z_Free(string);
}


// =====================================================================

#define MEGS_DIV	(1.0/1048576)


/*
 =================
 Com_MemInfo_f
 =================
*/
void Com_MemInfo_f (void){

	hunk_t		*h;
	memBlock_t	*block;

	// Debug tool for memory integrity checking and block information
	if (com_debugMemory->integer){
		Z_CheckHeap(smallZone);
		Z_CheckHeap(mainZone);
		Hunk_Check();

		Com_Printf("SMALL ZONE\n");
		Com_Printf("----------\n");
		for (block = smallZone->blockList.next; block->next != &smallZone->blockList; block = block->next)
			Com_Printf("   block: %8p   size: %8i   tag: %3i\n", block, block->size, block->tag);

		Com_Printf("MAIN ZONE\n");
		Com_Printf("---------\n");
		for (block = mainZone->blockList.next; block->next != &mainZone->blockList; block = block->next)
			Com_Printf("   block: %8p   size: %8i   tag: %3i\n", block, block->size, block->tag);

		Com_Printf("LOW HUNK\n");
		Com_Printf("--------\n");
		for (h = (hunk_t *)hunk_base; (byte *)h != hunk_base + hunk_lowUsed; h = (hunk_t *)((byte *)h + h->size))
			Com_Printf("   block: %8p   size: %8i\n", h, h->size);

		Com_Printf("HIGH HUNK\n");
		Com_Printf("---------\n");
		for (h = (hunk_t *)(hunk_base + hunk_size - hunk_highUsed); (byte *)h != hunk_base + hunk_size; h = (hunk_t *)((byte *)h + h->size))
			Com_Printf("   block: %8p   size: %8i\n", h, h->size);
	}

	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) total hunk\n", hunk_size, hunk_size * MEGS_DIV);
	Com_Printf("%9i bytes (%6.2f MB) total main zone\n", mainZone->size, mainZone->size * MEGS_DIV);
	Com_Printf("%9i bytes (%6.2f MB) total small zone\n", smallZone->size, smallZone->size * MEGS_DIV);
	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) low mark\n", hunk_lowMark, hunk_lowMark * MEGS_DIV);
	Com_Printf("%9i bytes (%6.2f MB) low used\n", hunk_lowUsed, hunk_lowUsed * MEGS_DIV);
	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) high mark\n", hunk_highMark, hunk_highMark * MEGS_DIV);
	Com_Printf("%9i bytes (%6.2f MB) high used\n", hunk_highUsed, hunk_highUsed * MEGS_DIV);
	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) hunk in use\n", hunk_lowUsed + hunk_highUsed, (hunk_lowUsed + hunk_highUsed) * MEGS_DIV);
	Com_Printf("%9i bytes (%6.2f MB) unused hunk\n", hunk_size - hunk_lowUsed - hunk_highUsed, (hunk_size - hunk_lowUsed - hunk_highUsed) * MEGS_DIV);
	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) in %i main zone blocks\n", mainZone->bytes, mainZone->bytes * MEGS_DIV, mainZone->blocks);
	Com_Printf("%9i bytes (%6.2f MB) free main zone\n", mainZone->size - mainZone->bytes, (mainZone->size - mainZone->bytes) * MEGS_DIV);
	Com_Printf("\n");
	Com_Printf("%9i bytes (%6.2f MB) in %i small zone blocks\n", smallZone->bytes, smallZone->bytes * MEGS_DIV, smallZone->blocks);
	Com_Printf("%9i bytes (%6.2f MB) free small zone\n", smallZone->size - smallZone->bytes, (smallZone->size - smallZone->bytes) * MEGS_DIV);
	Com_Printf("\n");
}

/*
 =================
 Com_TouchMemory

 Touch all the memory to make sure it's there
 =================
*/
void Com_TouchMemory (void){

	memBlock_t	*block;
	byte		*buffer;
	int			i, pagedTotal;

	// Run memory integrity checks
	Z_CheckHeap(smallZone);
	Z_CheckHeap(mainZone);
	Hunk_Check();

	// Small zone
	for (block = smallZone->blockList.next; block->next != &smallZone->blockList; block = block->next){
		if (block->tag == 0)
			continue;

		buffer = (byte *)block;
		for (i = 0; i < block->size; i += 4096)
			pagedTotal += buffer[i];
	}

	// Main zone
	for (block = mainZone->blockList.next; block->next != &mainZone->blockList; block = block->next){
		if (block->tag == 0)
			continue;

		buffer = (byte *)block;
		for (i = 0; i < block->size; i += 4096)
			pagedTotal += buffer[i];
	}

	// Low hunk
	buffer = hunk_base;
	for (i = 0; i < hunk_lowUsed; i += 4096)
		pagedTotal += buffer[i];

	// High hunk
	buffer = hunk_base + hunk_size - hunk_highUsed;
	for (i = 0; i < hunk_highUsed; i += 4096)
		pagedTotal += buffer[i];
}

/*
 =================
 Com_InitMemory
 =================
*/
void Com_InitMemory (void){

	int		size;

	if (!smallZone){
		// Initialize small zone memory
		smallZone = malloc(SMALLZONE_SIZE);
		if (!smallZone)
			Com_Error(ERR_FATAL, "Com_InitMemory: insufficient memory");

		Z_ClearZone(smallZone, SMALLZONE_SIZE);
		return;
	}

	// Initialize main zone memory
	if (com_zoneMegs->integer < MIN_ZONE_MEGS){
		Com_Printf("WARNING: minimum com_zoneMegs is %i, allocating %i MB\n", MIN_ZONE_MEGS, MIN_ZONE_MEGS);
		Cvar_ForceSet("com_zoneMegs", va("%i", MIN_ZONE_MEGS));
	}

	size = com_zoneMegs->integer << 20;

	while (1){
		mainZone = malloc(size);
		if (mainZone)
			break;

		size -= 0x400000;
		if (size < MIN_ZONE_SIZE)
			Com_Error(ERR_FATAL, "Com_InitMemory: insufficient memory");
	}

	if (com_zoneMegs->integer << 20 != size)
		Com_Printf("WARNING: couldn't allocate requested size of %i MB for zone memory, allocated %i MB\n", com_zoneMegs->integer, size >> 20);

	Z_ClearZone(mainZone, size);

	// Initialize hunk memory
	if (com_hunkMegs->integer < MIN_HUNK_MEGS){
		Com_Printf("WARNING: minimum com_hunkMegs is %i, allocating %i MB\n", MIN_HUNK_MEGS, MIN_HUNK_MEGS);
		Cvar_ForceSet("com_hunkMegs", va("%i", MIN_HUNK_MEGS));
	}

	hunk_size = com_hunkMegs->integer << 20;

	while (1){
		hunk_base = malloc(hunk_size);
		if (hunk_base)
			break;

		hunk_size -= 0x400000;
		if (hunk_size < MIN_HUNK_SIZE)
			Com_Error(ERR_FATAL, "Com_InitMemory: insufficient memory");
	}

	if (com_hunkMegs->integer << 20 != hunk_size)
		Com_Printf("WARNING: couldn't allocate requested size of %i MB for hunk memory, allocated %i MB\n", com_hunkMegs->integer, hunk_size >> 20);

	Hunk_Clear();
}

/*
 =================
 Com_ShutdownMemory
 =================
*/
void Com_ShutdownMemory (void){

	if (smallZone)
		free(smallZone);
	
	if (mainZone)
		free(mainZone);

	if (hunk_base)
		free(hunk_base);
}
