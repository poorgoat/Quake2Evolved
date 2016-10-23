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


#ifndef __QGL_H__
#define __QGL_H__


qboolean	QGL_Init (const char *driver);
void		QGL_Shutdown (void);
void		QGL_EnableLogging (qboolean enable);
void		QGL_LogPrintf (const char *fmt, ...);

#ifndef APIENTRY
#define APIENTRY
#endif

extern GLvoid			(APIENTRY * qglAccum)(GLenum op, GLfloat value);
extern GLvoid			(APIENTRY * qglAlphaFunc)(GLenum func, GLclampf ref);
extern GLboolean		(APIENTRY * qglAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
extern GLvoid			(APIENTRY * qglArrayElement)(GLint i);
extern GLvoid			(APIENTRY * qglBegin)(GLenum mode);
extern GLvoid			(APIENTRY * qglBindTexture)(GLenum target, GLuint texture);
extern GLvoid			(APIENTRY * qglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern GLvoid			(APIENTRY * qglBlendFunc)(GLenum sfactor, GLenum dfactor);
extern GLvoid			(APIENTRY * qglCallList)(GLuint list);
extern GLvoid			(APIENTRY * qglCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
extern GLvoid			(APIENTRY * qglClear)(GLbitfield mask);
extern GLvoid			(APIENTRY * qglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern GLvoid			(APIENTRY * qglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern GLvoid			(APIENTRY * qglClearDepth)(GLclampd depth);
extern GLvoid			(APIENTRY * qglClearIndex)(GLfloat c);
extern GLvoid			(APIENTRY * qglClearStencil)(GLint s);
extern GLvoid			(APIENTRY * qglClipPlane)(GLenum plane, const GLdouble *equation);
extern GLvoid			(APIENTRY * qglColor3b)(GLbyte red, GLbyte green, GLbyte blue);
extern GLvoid			(APIENTRY * qglColor3bv)(const GLbyte *v);
extern GLvoid			(APIENTRY * qglColor3d)(GLdouble red, GLdouble green, GLdouble blue);
extern GLvoid			(APIENTRY * qglColor3dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
extern GLvoid			(APIENTRY * qglColor3fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglColor3i)(GLint red, GLint green, GLint blue);
extern GLvoid			(APIENTRY * qglColor3iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglColor3s)(GLshort red, GLshort green, GLshort blue);
extern GLvoid			(APIENTRY * qglColor3sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
extern GLvoid			(APIENTRY * qglColor3ubv)(const GLubyte *v);
extern GLvoid			(APIENTRY * qglColor3ui)(GLuint red, GLuint green, GLuint blue);
extern GLvoid			(APIENTRY * qglColor3uiv)(const GLuint *v);
extern GLvoid			(APIENTRY * qglColor3us)(GLushort red, GLushort green, GLushort blue);
extern GLvoid			(APIENTRY * qglColor3usv)(const GLushort *v);
extern GLvoid			(APIENTRY * qglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern GLvoid			(APIENTRY * qglColor4bv)(const GLbyte *v);
extern GLvoid			(APIENTRY * qglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern GLvoid			(APIENTRY * qglColor4dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern GLvoid			(APIENTRY * qglColor4fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
extern GLvoid			(APIENTRY * qglColor4iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern GLvoid			(APIENTRY * qglColor4sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern GLvoid			(APIENTRY * qglColor4ubv)(const GLubyte *v);
extern GLvoid			(APIENTRY * qglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern GLvoid			(APIENTRY * qglColor4uiv)(const GLuint *v);
extern GLvoid			(APIENTRY * qglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern GLvoid			(APIENTRY * qglColor4usv)(const GLushort *v);
extern GLvoid			(APIENTRY * qglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern GLvoid			(APIENTRY * qglColorMaterial)(GLenum face, GLenum mode);
extern GLvoid			(APIENTRY * qglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern GLvoid			(APIENTRY * qglCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern GLvoid			(APIENTRY * qglCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern GLvoid			(APIENTRY * qglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern GLvoid			(APIENTRY * qglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern GLvoid			(APIENTRY * qglCullFace)(GLenum mode);
extern GLvoid			(APIENTRY * qglDeleteLists)(GLuint list, GLsizei range);
extern GLvoid			(APIENTRY * qglDeleteTextures)(GLsizei n, const GLuint *textures);
extern GLvoid			(APIENTRY * qglDepthFunc)(GLenum func);
extern GLvoid			(APIENTRY * qglDepthMask)(GLboolean flag);
extern GLvoid			(APIENTRY * qglDepthRange)(GLclampd zNear, GLclampd zFar);
extern GLvoid			(APIENTRY * qglDisable)(GLenum cap);
extern GLvoid			(APIENTRY * qglDisableClientState)(GLenum array);
extern GLvoid			(APIENTRY * qglDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern GLvoid			(APIENTRY * qglDrawBuffer)(GLenum mode);
extern GLvoid			(APIENTRY * qglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern GLvoid			(APIENTRY * qglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid			(APIENTRY * qglEdgeFlag)(GLboolean flag);
extern GLvoid			(APIENTRY * qglEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglEdgeFlagv)(const GLboolean *flag);
extern GLvoid			(APIENTRY * qglEnable)(GLenum cap);
extern GLvoid			(APIENTRY * qglEnableClientState)(GLenum array);
extern GLvoid			(APIENTRY * qglEnd)(GLvoid);
extern GLvoid			(APIENTRY * qglEndList)(GLvoid);
extern GLvoid			(APIENTRY * qglEvalCoord1d)(GLdouble u);
extern GLvoid			(APIENTRY * qglEvalCoord1dv)(const GLdouble *u);
extern GLvoid			(APIENTRY * qglEvalCoord1f)(GLfloat u);
extern GLvoid			(APIENTRY * qglEvalCoord1fv)(const GLfloat *u);
extern GLvoid			(APIENTRY * qglEvalCoord2d)(GLdouble u, GLdouble v);
extern GLvoid			(APIENTRY * qglEvalCoord2dv)(const GLdouble *u);
extern GLvoid			(APIENTRY * qglEvalCoord2f)(GLfloat u, GLfloat v);
extern GLvoid			(APIENTRY * qglEvalCoord2fv)(const GLfloat *u);
extern GLvoid			(APIENTRY * qglEvalMesh1)(GLenum mode, GLint i1, GLint i2);
extern GLvoid			(APIENTRY * qglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern GLvoid			(APIENTRY * qglEvalPoint1)(GLint i);
extern GLvoid			(APIENTRY * qglEvalPoint2)(GLint i, GLint j);
extern GLvoid			(APIENTRY * qglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
extern GLvoid			(APIENTRY * qglFinish)(GLvoid);
extern GLvoid			(APIENTRY * qglFlush)(GLvoid);
extern GLvoid			(APIENTRY * qglFogf)(GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglFogfv)(GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglFogi)(GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglFogiv)(GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglFrontFace)(GLenum mode);
extern GLvoid			(APIENTRY * qglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern GLuint			(APIENTRY * qglGenLists)(GLsizei range);
extern GLvoid			(APIENTRY * qglGenTextures)(GLsizei n, GLuint *textures);
extern GLvoid			(APIENTRY * qglGetBooleanv)(GLenum pname, GLboolean *params);
extern GLvoid			(APIENTRY * qglGetClipPlane)(GLenum plane, GLdouble *equation);
extern GLvoid			(APIENTRY * qglGetDoublev)(GLenum pname, GLdouble *params);
extern GLenum			(APIENTRY * qglGetError)(GLvoid);
extern GLvoid			(APIENTRY * qglGetFloatv)(GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetIntegerv)(GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetLightiv)(GLenum light, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetMapdv)(GLenum target, GLenum query, GLdouble *v);
extern GLvoid			(APIENTRY * qglGetMapfv)(GLenum target, GLenum query, GLfloat *v);
extern GLvoid			(APIENTRY * qglGetMapiv)(GLenum target, GLenum query, GLint *v);
extern GLvoid			(APIENTRY * qglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetPixelMapfv)(GLenum map, GLfloat *values);
extern GLvoid			(APIENTRY * qglGetPixelMapuiv)(GLenum map, GLuint *values);
extern GLvoid			(APIENTRY * qglGetPixelMapusv)(GLenum map, GLushort *values);
extern GLvoid			(APIENTRY * qglGetPointerv)(GLenum pname, GLvoid* *params);
extern GLvoid			(APIENTRY * qglGetPolygonStipple)(GLubyte *mask);
extern const GLubyte *	(APIENTRY * qglGetString)(GLenum name);
extern GLvoid			(APIENTRY * qglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
extern GLvoid			(APIENTRY * qglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern GLvoid			(APIENTRY * qglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern GLvoid			(APIENTRY * qglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
extern GLvoid			(APIENTRY * qglHint)(GLenum target, GLenum mode);
extern GLvoid			(APIENTRY * qglIndexMask)(GLuint mask);
extern GLvoid			(APIENTRY * qglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglIndexd)(GLdouble c);
extern GLvoid			(APIENTRY * qglIndexdv)(const GLdouble *c);
extern GLvoid			(APIENTRY * qglIndexf)(GLfloat c);
extern GLvoid			(APIENTRY * qglIndexfv)(const GLfloat *c);
extern GLvoid			(APIENTRY * qglIndexi)(GLint c);
extern GLvoid			(APIENTRY * qglIndexiv)(const GLint *c);
extern GLvoid			(APIENTRY * qglIndexs)(GLshort c);
extern GLvoid			(APIENTRY * qglIndexsv)(const GLshort *c);
extern GLvoid			(APIENTRY * qglIndexub)(GLubyte c);
extern GLvoid			(APIENTRY * qglIndexubv)(const GLubyte *c);
extern GLvoid			(APIENTRY * qglInitNames)(GLvoid);
extern GLvoid			(APIENTRY * qglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
extern GLboolean		(APIENTRY * qglIsEnabled)(GLenum cap);
extern GLboolean		(APIENTRY * qglIsList)(GLuint list);
extern GLboolean		(APIENTRY * qglIsTexture)(GLuint texture);
extern GLvoid			(APIENTRY * qglLightModelf)(GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglLightModelfv)(GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglLightModeli)(GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglLightModeliv)(GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglLightf)(GLenum light, GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglLightfv)(GLenum light, GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglLighti)(GLenum light, GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglLightiv)(GLenum light, GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglLineStipple)(GLint factor, GLushort pattern);
extern GLvoid			(APIENTRY * qglLineWidth)(GLfloat width);
extern GLvoid			(APIENTRY * qglListBase)(GLuint base);
extern GLvoid			(APIENTRY * qglLoadIdentity)(GLvoid);
extern GLvoid			(APIENTRY * qglLoadMatrixd)(const GLdouble *m);
extern GLvoid			(APIENTRY * qglLoadMatrixf)(const GLfloat *m);
extern GLvoid			(APIENTRY * qglLoadName)(GLuint name);
extern GLvoid			(APIENTRY * qglLogicOp)(GLenum opcode);
extern GLvoid			(APIENTRY * qglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern GLvoid			(APIENTRY * qglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern GLvoid			(APIENTRY * qglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern GLvoid			(APIENTRY * qglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern GLvoid			(APIENTRY * qglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
extern GLvoid			(APIENTRY * qglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
extern GLvoid			(APIENTRY * qglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern GLvoid			(APIENTRY * qglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern GLvoid			(APIENTRY * qglMaterialf)(GLenum face, GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglMateriali)(GLenum face, GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglMaterialiv)(GLenum face, GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglMatrixMode)(GLenum mode);
extern GLvoid			(APIENTRY * qglMultMatrixd)(const GLdouble *m);
extern GLvoid			(APIENTRY * qglMultMatrixf)(const GLfloat *m);
extern GLvoid			(APIENTRY * qglNewList)(GLuint list, GLenum mode);
extern GLvoid			(APIENTRY * qglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
extern GLvoid			(APIENTRY * qglNormal3bv)(const GLbyte *v);
extern GLvoid			(APIENTRY * qglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
extern GLvoid			(APIENTRY * qglNormal3dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
extern GLvoid			(APIENTRY * qglNormal3fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglNormal3i)(GLint nx, GLint ny, GLint nz);
extern GLvoid			(APIENTRY * qglNormal3iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglNormal3s)(GLshort nx, GLshort ny, GLshort nz);
extern GLvoid			(APIENTRY * qglNormal3sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern GLvoid			(APIENTRY * qglPassThrough)(GLfloat token);
extern GLvoid			(APIENTRY * qglPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
extern GLvoid			(APIENTRY * qglPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
extern GLvoid			(APIENTRY * qglPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
extern GLvoid			(APIENTRY * qglPixelStoref)(GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglPixelStorei)(GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglPixelTransferf)(GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglPixelTransferi)(GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglPixelZoom)(GLfloat xfactor, GLfloat yfactor);
extern GLvoid			(APIENTRY * qglPointSize)(GLfloat size);
extern GLvoid			(APIENTRY * qglPolygonMode)(GLenum face, GLenum mode);
extern GLvoid			(APIENTRY * qglPolygonOffset)(GLfloat factor, GLfloat units);
extern GLvoid			(APIENTRY * qglPolygonStipple)(const GLubyte *mask);
extern GLvoid			(APIENTRY * qglPopAttrib)(GLvoid);
extern GLvoid			(APIENTRY * qglPopClientAttrib)(GLvoid);
extern GLvoid			(APIENTRY * qglPopMatrix)(GLvoid);
extern GLvoid			(APIENTRY * qglPopName)(GLvoid);
extern GLvoid			(APIENTRY * qglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern GLvoid			(APIENTRY * qglPushAttrib)(GLbitfield mask);
extern GLvoid			(APIENTRY * qglPushClientAttrib)(GLbitfield mask);
extern GLvoid			(APIENTRY * qglPushMatrix)(GLvoid);
extern GLvoid			(APIENTRY * qglPushName)(GLuint name);
extern GLvoid			(APIENTRY * qglRasterPos2d)(GLdouble x, GLdouble y);
extern GLvoid			(APIENTRY * qglRasterPos2dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglRasterPos2f)(GLfloat x, GLfloat y);
extern GLvoid			(APIENTRY * qglRasterPos2fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglRasterPos2i)(GLint x, GLint y);
extern GLvoid			(APIENTRY * qglRasterPos2iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglRasterPos2s)(GLshort x, GLshort y);
extern GLvoid			(APIENTRY * qglRasterPos2sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
extern GLvoid			(APIENTRY * qglRasterPos3dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
extern GLvoid			(APIENTRY * qglRasterPos3fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglRasterPos3i)(GLint x, GLint y, GLint z);
extern GLvoid			(APIENTRY * qglRasterPos3iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglRasterPos3s)(GLshort x, GLshort y, GLshort z);
extern GLvoid			(APIENTRY * qglRasterPos3sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern GLvoid			(APIENTRY * qglRasterPos4dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern GLvoid			(APIENTRY * qglRasterPos4fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
extern GLvoid			(APIENTRY * qglRasterPos4iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
extern GLvoid			(APIENTRY * qglRasterPos4sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglReadBuffer)(GLenum mode);
extern GLvoid			(APIENTRY * qglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern GLvoid			(APIENTRY * qglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern GLvoid			(APIENTRY * qglRectdv)(const GLdouble *v1, const GLdouble *v2);
extern GLvoid			(APIENTRY * qglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern GLvoid			(APIENTRY * qglRectfv)(const GLfloat *v1, const GLfloat *v2);
extern GLvoid			(APIENTRY * qglRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
extern GLvoid			(APIENTRY * qglRectiv)(const GLint *v1, const GLint *v2);
extern GLvoid			(APIENTRY * qglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern GLvoid			(APIENTRY * qglRectsv)(const GLshort *v1, const GLshort *v2);
extern GLint			(APIENTRY * qglRenderMode)(GLenum mode);
extern GLvoid			(APIENTRY * qglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern GLvoid			(APIENTRY * qglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern GLvoid			(APIENTRY * qglScaled)(GLdouble x, GLdouble y, GLdouble z);
extern GLvoid			(APIENTRY * qglScalef)(GLfloat x, GLfloat y, GLfloat z);
extern GLvoid			(APIENTRY * qglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
extern GLvoid			(APIENTRY * qglSelectBuffer)(GLsizei size, GLuint *buffer);
extern GLvoid			(APIENTRY * qglShadeModel)(GLenum mode);
extern GLvoid			(APIENTRY * qglStencilFunc)(GLenum func, GLint ref, GLuint mask);
extern GLvoid			(APIENTRY * qglStencilMask)(GLuint mask);
extern GLvoid			(APIENTRY * qglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
extern GLvoid			(APIENTRY * qglTexCoord1d)(GLdouble s);
extern GLvoid			(APIENTRY * qglTexCoord1dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglTexCoord1f)(GLfloat s);
extern GLvoid			(APIENTRY * qglTexCoord1fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglTexCoord1i)(GLint s);
extern GLvoid			(APIENTRY * qglTexCoord1iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglTexCoord1s)(GLshort s);
extern GLvoid			(APIENTRY * qglTexCoord1sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglTexCoord2d)(GLdouble s, GLdouble t);
extern GLvoid			(APIENTRY * qglTexCoord2dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglTexCoord2f)(GLfloat s, GLfloat t);
extern GLvoid			(APIENTRY * qglTexCoord2fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglTexCoord2i)(GLint s, GLint t);
extern GLvoid			(APIENTRY * qglTexCoord2iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglTexCoord2s)(GLshort s, GLshort t);
extern GLvoid			(APIENTRY * qglTexCoord2sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
extern GLvoid			(APIENTRY * qglTexCoord3dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
extern GLvoid			(APIENTRY * qglTexCoord3fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglTexCoord3i)(GLint s, GLint t, GLint r);
extern GLvoid			(APIENTRY * qglTexCoord3iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglTexCoord3s)(GLshort s, GLshort t, GLshort r);
extern GLvoid			(APIENTRY * qglTexCoord3sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern GLvoid			(APIENTRY * qglTexCoord4dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern GLvoid			(APIENTRY * qglTexCoord4fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
extern GLvoid			(APIENTRY * qglTexCoord4iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
extern GLvoid			(APIENTRY * qglTexCoord4sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglTexEnvi)(GLenum target, GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglTexEnviv)(GLenum target, GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglTexGend)(GLenum coord, GLenum pname, GLdouble param);
extern GLvoid			(APIENTRY * qglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
extern GLvoid			(APIENTRY * qglTexGenf)(GLenum coord, GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglTexGeni)(GLenum coord, GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid			(APIENTRY * qglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid			(APIENTRY * qglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
extern GLvoid			(APIENTRY * qglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
extern GLvoid			(APIENTRY * qglTexParameteri)(GLenum target, GLenum pname, GLint param);
extern GLvoid			(APIENTRY * qglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
extern GLvoid			(APIENTRY * qglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid			(APIENTRY * qglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid			(APIENTRY * qglTranslated)(GLdouble x, GLdouble y, GLdouble z);
extern GLvoid			(APIENTRY * qglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
extern GLvoid			(APIENTRY * qglVertex2d)(GLdouble x, GLdouble y);
extern GLvoid			(APIENTRY * qglVertex2dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglVertex2f)(GLfloat x, GLfloat y);
extern GLvoid			(APIENTRY * qglVertex2fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglVertex2i)(GLint x, GLint y);
extern GLvoid			(APIENTRY * qglVertex2iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglVertex2s)(GLshort x, GLshort y);
extern GLvoid			(APIENTRY * qglVertex2sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
extern GLvoid			(APIENTRY * qglVertex3dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
extern GLvoid			(APIENTRY * qglVertex3fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglVertex3i)(GLint x, GLint y, GLint z);
extern GLvoid			(APIENTRY * qglVertex3iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglVertex3s)(GLshort x, GLshort y, GLshort z);
extern GLvoid			(APIENTRY * qglVertex3sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern GLvoid			(APIENTRY * qglVertex4dv)(const GLdouble *v);
extern GLvoid			(APIENTRY * qglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern GLvoid			(APIENTRY * qglVertex4fv)(const GLfloat *v);
extern GLvoid			(APIENTRY * qglVertex4i)(GLint x, GLint y, GLint z, GLint w);
extern GLvoid			(APIENTRY * qglVertex4iv)(const GLint *v);
extern GLvoid			(APIENTRY * qglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
extern GLvoid			(APIENTRY * qglVertex4sv)(const GLshort *v);
extern GLvoid			(APIENTRY * qglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern GLvoid			(APIENTRY * qglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

extern GLvoid			(APIENTRY * qglActiveTextureARB)(GLenum texture);
extern GLvoid			(APIENTRY * qglClientActiveTextureARB)(GLenum texture);

extern GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
extern GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
extern GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
extern GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
extern GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
extern GLvoid *			(APIENTRY * qglMapBufferARB)(GLenum target, GLenum access);
extern GLboolean		(APIENTRY * qglUnmapBufferARB)(GLenum target);

extern GLvoid			(APIENTRY * qglBindProgramARB)(GLenum target, GLuint program);
extern GLvoid			(APIENTRY * qglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
extern GLvoid			(APIENTRY * qglGenProgramsARB)(GLsizei n, GLuint *programs);
extern GLvoid			(APIENTRY * qglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
extern GLvoid			(APIENTRY * qglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern GLvoid			(APIENTRY * qglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern GLvoid			(APIENTRY * qglDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

extern GLvoid			(APIENTRY * qglLockArraysEXT)(GLint start, GLsizei count);
extern GLvoid			(APIENTRY * qglUnlockArraysEXT)(GLvoid);

extern GLvoid			(APIENTRY * qglActiveStencilFaceEXT)(GLenum face);

extern GLvoid			(APIENTRY * qglStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
extern GLvoid			(APIENTRY * qglStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint red, GLuint mask);

// =====================================================================

#ifdef _WIN32

extern int				(WINAPI   * qwglChoosePixelFormat)(HDC hDC, CONST PIXELFORMATDESCRIPTOR *pPFD);
extern int				(WINAPI   * qwglDescribePixelFormat)(HDC hDC, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pPFD);
extern int				(WINAPI   * qwglGetPixelFormat)(HDC hDC);
extern BOOL				(WINAPI   * qwglSetPixelFormat)(HDC hDC, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *pPFD);
extern BOOL				(WINAPI   * qwglSwapBuffers)(HDC hDC);

extern BOOL				(WINAPI   * qwglCopyContext)(HGLRC hGLRCSrc, HGLRC hGLRCDst, UINT mask);
extern HGLRC			(WINAPI   * qwglCreateContext)(HDC hDC);
extern HGLRC			(WINAPI   * qwglCreateLayerContext)(HDC hDC, int iLayerPlane);
extern BOOL				(WINAPI   * qwglDeleteContext)(HGLRC hGLRC);
extern HGLRC			(WINAPI   * qwglGetCurrentContext)(void);
extern HDC				(WINAPI   * qwglGetCurrentDC)(void);
extern PROC				(WINAPI   * qwglGetProcAddress)(LPCSTR lpszProc);
extern BOOL				(WINAPI   * qwglMakeCurrent)(HDC hDC, HGLRC hGLRC);
extern BOOL				(WINAPI   * qwglShareLists)(HGLRC hGLRC1, HGLRC hGLRC2);
extern BOOL				(WINAPI   * qwglUseFontBitmaps)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
extern BOOL				(WINAPI   * qwglUseFontOutlines)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpGMF);
extern BOOL				(WINAPI   * qwglDescribeLayerPlane)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR pLPD);
extern int				(WINAPI   * qwglSetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pCR);
extern int				(WINAPI   * qwglGetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, COLORREF *pCR);
extern BOOL				(WINAPI   * qwglRealizeLayerPalette)(HDC hDC, int iLayerPlane, BOOL bRealize);
extern BOOL				(WINAPI   * qwglSwapLayerBuffers)(HDC hDC, UINT fuPlanes);

extern BOOL				(WINAPI   * qwglSwapIntervalEXT)(int interval);

#endif

#ifdef __linux__

extern void				*qglXGetProcAddress (char *symbol);

extern XVisualInfo *	(*qglXChooseVisual)(Display *dpy, int screen, int *attribList);
extern GLXContext		(*qglXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
extern void				(*qglXDestroyContext)(Display *dpy, GLXContext ctx);
extern Bool				(*qglXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void				(*qglXCopyContext)(Display *dpy, GLXContext src, GLXContext dst, GLuint mask);
extern void				(*qglXSwapBuffers)(Display *dpy, GLXDrawable drawable);

#endif


#endif	// __QGL_H__
