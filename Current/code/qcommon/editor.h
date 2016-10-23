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


#ifndef __EDITOR_H__
#define __EDITOR_H__


typedef struct {
	vec3_t			origin;
	vec3_t			center;
	vec3_t			angles;
	vec3_t			radius;

	int				style;
	int				detailLevel;
	qboolean		parallel;
	qboolean		noShadows;

	char			material[MAX_OSPATH];
	float			materialParms[8];
} lightProperties_t;

void	E_CreateLightPropertiesWindow (void);
void	E_DestroyLightPropertiesWindow (void);
void	E_SetLightPropertiesCallbacks (void (*updateLight)(lightProperties_t *), void (*removeLight)(void), void (*closeLight)(void));
void	E_AddLightPropertiesMaterial (const char *name);
void	E_EditLightProperties (lightProperties_t *lightProperties);
void	E_ApplyLightProperties (void);


#endif	// __EDITOR_H__
