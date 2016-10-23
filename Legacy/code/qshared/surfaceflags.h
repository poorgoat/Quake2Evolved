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


#ifndef __SURFACEFLAGS_H__
#define __SURFACEFLAGS_H__


// Contents flags are separate bits.
// A given brush can contribute multiple content bits.
// Multiple brushes can be in a single leaf.

// Lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			0x00000001	// An eye is never valid in a solid
#define	CONTENTS_WINDOW			0x00000002	// Translucent, but not watery
#define	CONTENTS_AUX			0x00000004
#define	CONTENTS_LAVA			0x00000008
#define	CONTENTS_SLIME			0x00000010
#define	CONTENTS_WATER			0x00000020
#define	CONTENTS_MIST			0x00000040

// Remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x00008000

#define	CONTENTS_PLAYERCLIP		0x00010000
#define	CONTENTS_MONSTERCLIP	0x00020000

// Currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x00040000
#define	CONTENTS_CURRENT_90		0x00080000
#define	CONTENTS_CURRENT_180	0x00100000
#define	CONTENTS_CURRENT_270	0x00200000
#define	CONTENTS_CURRENT_UP		0x00400000
#define	CONTENTS_CURRENT_DOWN	0x00800000

#define	CONTENTS_ORIGIN			0x01000000	// Removed before BSP'ing an entity

#define	CONTENTS_MONSTER		0x02000000	// Should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x04000000
#define	CONTENTS_DETAIL			0x08000000	// Brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// Auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

#define	SURF_LIGHT				0x00000001	// Value will hold the light strength
#define	SURF_SLICK				0x00000002	// Effects game physics
#define	SURF_SKY				0x00000004	// Don't draw, but add to skybox
#define	SURF_WARP				0x00000008	// Turbulent water warp
#define	SURF_TRANS33			0x00000010	// 33% opacity
#define	SURF_TRANS66			0x00000020	// 66% opacity
#define	SURF_FLOWING			0x00000040	// Scroll towards angle
#define	SURF_NODRAW				0x00000080	// Don't bother referencing the texture


#endif	// __SURFACEFLAGS_H__
