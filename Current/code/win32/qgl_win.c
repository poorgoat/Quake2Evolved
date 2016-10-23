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


// qgl_win.c -- binding of GL to QGL function pointers


#include "../render/r_local.h"


int				(WINAPI   * qwglChoosePixelFormat)(HDC hDC, CONST PIXELFORMATDESCRIPTOR *pPFD);
int				(WINAPI   * qwglDescribePixelFormat)(HDC hDC, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pPFD);
int				(WINAPI   * qwglGetPixelFormat)(HDC hDC);
BOOL			(WINAPI   * qwglSetPixelFormat)(HDC hDC, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *pPFD);
BOOL			(WINAPI   * qwglSwapBuffers)(HDC hDC);

BOOL			(WINAPI   * qwglCopyContext)(HGLRC hGLRCSrc, HGLRC hGLRCDst, UINT mask);
HGLRC			(WINAPI   * qwglCreateContext)(HDC hDC);
HGLRC			(WINAPI   * qwglCreateLayerContext)(HDC hDC, int iLayerPlane);
BOOL			(WINAPI   * qwglDeleteContext)(HGLRC hGLRC);
HGLRC			(WINAPI   * qwglGetCurrentContext)(void);
HDC				(WINAPI   * qwglGetCurrentDC)(void);
PROC			(WINAPI   * qwglGetProcAddress)(LPCSTR lpszProc);
BOOL			(WINAPI   * qwglMakeCurrent)(HDC hDC, HGLRC hGLRC);
BOOL			(WINAPI   * qwglShareLists)(HGLRC hGLRC1, HGLRC hGLRC2);
BOOL			(WINAPI   * qwglUseFontBitmaps)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
BOOL			(WINAPI   * qwglUseFontOutlines)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpGMF);
BOOL			(WINAPI   * qwglDescribeLayerPlane)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR pLPD);
int				(WINAPI   * qwglSetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pCR);
int				(WINAPI   * qwglGetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, COLORREF *pCR);
BOOL			(WINAPI   * qwglRealizeLayerPalette)(HDC hDC, int iLayerPlane, BOOL bRealize);
BOOL			(WINAPI   * qwglSwapLayerBuffers)(HDC hDC, UINT fuPlanes);

GLvoid			(APIENTRY * qglAccum)(GLenum op, GLfloat value);
GLvoid			(APIENTRY * qglAlphaFunc)(GLenum func, GLclampf ref);
GLboolean		(APIENTRY * qglAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
GLvoid			(APIENTRY * qglArrayElement)(GLint i);
GLvoid			(APIENTRY * qglBegin)(GLenum mode);
GLvoid			(APIENTRY * qglBindTexture)(GLenum target, GLuint texture);
GLvoid			(APIENTRY * qglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
GLvoid			(APIENTRY * qglBlendFunc)(GLenum sfactor, GLenum dfactor);
GLvoid			(APIENTRY * qglCallList)(GLuint list);
GLvoid			(APIENTRY * qglCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
GLvoid			(APIENTRY * qglClear)(GLbitfield mask);
GLvoid			(APIENTRY * qglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLvoid			(APIENTRY * qglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GLvoid			(APIENTRY * qglClearDepth)(GLclampd depth);
GLvoid			(APIENTRY * qglClearIndex)(GLfloat c);
GLvoid			(APIENTRY * qglClearStencil)(GLint s);
GLvoid			(APIENTRY * qglClipPlane)(GLenum plane, const GLdouble *equation);
GLvoid			(APIENTRY * qglColor3b)(GLbyte red, GLbyte green, GLbyte blue);
GLvoid			(APIENTRY * qglColor3bv)(const GLbyte *v);
GLvoid			(APIENTRY * qglColor3d)(GLdouble red, GLdouble green, GLdouble blue);
GLvoid			(APIENTRY * qglColor3dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
GLvoid			(APIENTRY * qglColor3fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglColor3i)(GLint red, GLint green, GLint blue);
GLvoid			(APIENTRY * qglColor3iv)(const GLint *v);
GLvoid			(APIENTRY * qglColor3s)(GLshort red, GLshort green, GLshort blue);
GLvoid			(APIENTRY * qglColor3sv)(const GLshort *v);
GLvoid			(APIENTRY * qglColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
GLvoid			(APIENTRY * qglColor3ubv)(const GLubyte *v);
GLvoid			(APIENTRY * qglColor3ui)(GLuint red, GLuint green, GLuint blue);
GLvoid			(APIENTRY * qglColor3uiv)(const GLuint *v);
GLvoid			(APIENTRY * qglColor3us)(GLushort red, GLushort green, GLushort blue);
GLvoid			(APIENTRY * qglColor3usv)(const GLushort *v);
GLvoid			(APIENTRY * qglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
GLvoid			(APIENTRY * qglColor4bv)(const GLbyte *v);
GLvoid			(APIENTRY * qglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GLvoid			(APIENTRY * qglColor4dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLvoid			(APIENTRY * qglColor4fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
GLvoid			(APIENTRY * qglColor4iv)(const GLint *v);
GLvoid			(APIENTRY * qglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
GLvoid			(APIENTRY * qglColor4sv)(const GLshort *v);
GLvoid			(APIENTRY * qglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GLvoid			(APIENTRY * qglColor4ubv)(const GLubyte *v);
GLvoid			(APIENTRY * qglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
GLvoid			(APIENTRY * qglColor4uiv)(const GLuint *v);
GLvoid			(APIENTRY * qglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
GLvoid			(APIENTRY * qglColor4usv)(const GLushort *v);
GLvoid			(APIENTRY * qglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLvoid			(APIENTRY * qglColorMaterial)(GLenum face, GLenum mode);
GLvoid			(APIENTRY * qglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
GLvoid			(APIENTRY * qglCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
GLvoid			(APIENTRY * qglCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLvoid			(APIENTRY * qglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GLvoid			(APIENTRY * qglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLvoid			(APIENTRY * qglCullFace)(GLenum mode);
GLvoid			(APIENTRY * qglDeleteLists)(GLuint list, GLsizei range);
GLvoid			(APIENTRY * qglDeleteTextures)(GLsizei n, const GLuint *textures);
GLvoid			(APIENTRY * qglDepthFunc)(GLenum func);
GLvoid			(APIENTRY * qglDepthMask)(GLboolean flag);
GLvoid			(APIENTRY * qglDepthRange)(GLclampd zNear, GLclampd zFar);
GLvoid			(APIENTRY * qglDisable)(GLenum cap);
GLvoid			(APIENTRY * qglDisableClientState)(GLenum array);
GLvoid			(APIENTRY * qglDrawArrays)(GLenum mode, GLint first, GLsizei count);
GLvoid			(APIENTRY * qglDrawBuffer)(GLenum mode);
GLvoid			(APIENTRY * qglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GLvoid			(APIENTRY * qglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid			(APIENTRY * qglEdgeFlag)(GLboolean flag);
GLvoid			(APIENTRY * qglEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglEdgeFlagv)(const GLboolean *flag);
GLvoid			(APIENTRY * qglEnable)(GLenum cap);
GLvoid			(APIENTRY * qglEnableClientState)(GLenum array);
GLvoid			(APIENTRY * qglEnd)(GLvoid);
GLvoid			(APIENTRY * qglEndList)(GLvoid);
GLvoid			(APIENTRY * qglEvalCoord1d)(GLdouble u);
GLvoid			(APIENTRY * qglEvalCoord1dv)(const GLdouble *u);
GLvoid			(APIENTRY * qglEvalCoord1f)(GLfloat u);
GLvoid			(APIENTRY * qglEvalCoord1fv)(const GLfloat *u);
GLvoid			(APIENTRY * qglEvalCoord2d)(GLdouble u, GLdouble v);
GLvoid			(APIENTRY * qglEvalCoord2dv)(const GLdouble *u);
GLvoid			(APIENTRY * qglEvalCoord2f)(GLfloat u, GLfloat v);
GLvoid			(APIENTRY * qglEvalCoord2fv)(const GLfloat *u);
GLvoid			(APIENTRY * qglEvalMesh1)(GLenum mode, GLint i1, GLint i2);
GLvoid			(APIENTRY * qglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
GLvoid			(APIENTRY * qglEvalPoint1)(GLint i);
GLvoid			(APIENTRY * qglEvalPoint2)(GLint i, GLint j);
GLvoid			(APIENTRY * qglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
GLvoid			(APIENTRY * qglFinish)(GLvoid);
GLvoid			(APIENTRY * qglFlush)(GLvoid);
GLvoid			(APIENTRY * qglFogf)(GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglFogfv)(GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglFogi)(GLenum pname, GLint param);
GLvoid			(APIENTRY * qglFogiv)(GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglFrontFace)(GLenum mode);
GLvoid			(APIENTRY * qglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint			(APIENTRY * qglGenLists)(GLsizei range);
GLvoid			(APIENTRY * qglGenTextures)(GLsizei n, GLuint *textures);
GLvoid			(APIENTRY * qglGetBooleanv)(GLenum pname, GLboolean *params);
GLvoid			(APIENTRY * qglGetClipPlane)(GLenum plane, GLdouble *equation);
GLvoid			(APIENTRY * qglGetDoublev)(GLenum pname, GLdouble *params);
GLenum			(APIENTRY * qglGetError)(GLvoid);
GLvoid			(APIENTRY * qglGetFloatv)(GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetIntegerv)(GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetLightiv)(GLenum light, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetMapdv)(GLenum target, GLenum query, GLdouble *v);
GLvoid			(APIENTRY * qglGetMapfv)(GLenum target, GLenum query, GLfloat *v);
GLvoid			(APIENTRY * qglGetMapiv)(GLenum target, GLenum query, GLint *v);
GLvoid			(APIENTRY * qglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetPixelMapfv)(GLenum map, GLfloat *values);
GLvoid			(APIENTRY * qglGetPixelMapuiv)(GLenum map, GLuint *values);
GLvoid			(APIENTRY * qglGetPixelMapusv)(GLenum map, GLushort *values);
GLvoid			(APIENTRY * qglGetPointerv)(GLenum pname, GLvoid* *params);
GLvoid			(APIENTRY * qglGetPolygonStipple)(GLubyte *mask);
const GLubyte *	(APIENTRY * qglGetString)(GLenum name);
GLvoid			(APIENTRY * qglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
GLvoid			(APIENTRY * qglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
GLvoid			(APIENTRY * qglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
GLvoid			(APIENTRY * qglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
GLvoid			(APIENTRY * qglHint)(GLenum target, GLenum mode);
GLvoid			(APIENTRY * qglIndexMask)(GLuint mask);
GLvoid			(APIENTRY * qglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglIndexd)(GLdouble c);
GLvoid			(APIENTRY * qglIndexdv)(const GLdouble *c);
GLvoid			(APIENTRY * qglIndexf)(GLfloat c);
GLvoid			(APIENTRY * qglIndexfv)(const GLfloat *c);
GLvoid			(APIENTRY * qglIndexi)(GLint c);
GLvoid			(APIENTRY * qglIndexiv)(const GLint *c);
GLvoid			(APIENTRY * qglIndexs)(GLshort c);
GLvoid			(APIENTRY * qglIndexsv)(const GLshort *c);
GLvoid			(APIENTRY * qglIndexub)(GLubyte c);
GLvoid			(APIENTRY * qglIndexubv)(const GLubyte *c);
GLvoid			(APIENTRY * qglInitNames)(GLvoid);
GLvoid			(APIENTRY * qglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean		(APIENTRY * qglIsEnabled)(GLenum cap);
GLboolean		(APIENTRY * qglIsList)(GLuint list);
GLboolean		(APIENTRY * qglIsTexture)(GLuint texture);
GLvoid			(APIENTRY * qglLightModelf)(GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglLightModelfv)(GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglLightModeli)(GLenum pname, GLint param);
GLvoid			(APIENTRY * qglLightModeliv)(GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglLightf)(GLenum light, GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglLightfv)(GLenum light, GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglLighti)(GLenum light, GLenum pname, GLint param);
GLvoid			(APIENTRY * qglLightiv)(GLenum light, GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglLineStipple)(GLint factor, GLushort pattern);
GLvoid			(APIENTRY * qglLineWidth)(GLfloat width);
GLvoid			(APIENTRY * qglListBase)(GLuint base);
GLvoid			(APIENTRY * qglLoadIdentity)(GLvoid);
GLvoid			(APIENTRY * qglLoadMatrixd)(const GLdouble *m);
GLvoid			(APIENTRY * qglLoadMatrixf)(const GLfloat *m);
GLvoid			(APIENTRY * qglLoadName)(GLuint name);
GLvoid			(APIENTRY * qglLogicOp)(GLenum opcode);
GLvoid			(APIENTRY * qglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GLvoid			(APIENTRY * qglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GLvoid			(APIENTRY * qglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GLvoid			(APIENTRY * qglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
GLvoid			(APIENTRY * qglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
GLvoid			(APIENTRY * qglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
GLvoid			(APIENTRY * qglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
GLvoid			(APIENTRY * qglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
GLvoid			(APIENTRY * qglMaterialf)(GLenum face, GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglMateriali)(GLenum face, GLenum pname, GLint param);
GLvoid			(APIENTRY * qglMaterialiv)(GLenum face, GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglMatrixMode)(GLenum mode);
GLvoid			(APIENTRY * qglMultMatrixd)(const GLdouble *m);
GLvoid			(APIENTRY * qglMultMatrixf)(const GLfloat *m);
GLvoid			(APIENTRY * qglNewList)(GLuint list, GLenum mode);
GLvoid			(APIENTRY * qglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
GLvoid			(APIENTRY * qglNormal3bv)(const GLbyte *v);
GLvoid			(APIENTRY * qglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
GLvoid			(APIENTRY * qglNormal3dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
GLvoid			(APIENTRY * qglNormal3fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglNormal3i)(GLint nx, GLint ny, GLint nz);
GLvoid			(APIENTRY * qglNormal3iv)(const GLint *v);
GLvoid			(APIENTRY * qglNormal3s)(GLshort nx, GLshort ny, GLshort nz);
GLvoid			(APIENTRY * qglNormal3sv)(const GLshort *v);
GLvoid			(APIENTRY * qglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLvoid			(APIENTRY * qglPassThrough)(GLfloat token);
GLvoid			(APIENTRY * qglPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
GLvoid			(APIENTRY * qglPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
GLvoid			(APIENTRY * qglPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
GLvoid			(APIENTRY * qglPixelStoref)(GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglPixelStorei)(GLenum pname, GLint param);
GLvoid			(APIENTRY * qglPixelTransferf)(GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglPixelTransferi)(GLenum pname, GLint param);
GLvoid			(APIENTRY * qglPixelZoom)(GLfloat xfactor, GLfloat yfactor);
GLvoid			(APIENTRY * qglPointSize)(GLfloat size);
GLvoid			(APIENTRY * qglPolygonMode)(GLenum face, GLenum mode);
GLvoid			(APIENTRY * qglPolygonOffset)(GLfloat factor, GLfloat units);
GLvoid			(APIENTRY * qglPolygonStipple)(const GLubyte *mask);
GLvoid			(APIENTRY * qglPopAttrib)(GLvoid);
GLvoid			(APIENTRY * qglPopClientAttrib)(GLvoid);
GLvoid			(APIENTRY * qglPopMatrix)(GLvoid);
GLvoid			(APIENTRY * qglPopName)(GLvoid);
GLvoid			(APIENTRY * qglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
GLvoid			(APIENTRY * qglPushAttrib)(GLbitfield mask);
GLvoid			(APIENTRY * qglPushClientAttrib)(GLbitfield mask);
GLvoid			(APIENTRY * qglPushMatrix)(GLvoid);
GLvoid			(APIENTRY * qglPushName)(GLuint name);
GLvoid			(APIENTRY * qglRasterPos2d)(GLdouble x, GLdouble y);
GLvoid			(APIENTRY * qglRasterPos2dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglRasterPos2f)(GLfloat x, GLfloat y);
GLvoid			(APIENTRY * qglRasterPos2fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglRasterPos2i)(GLint x, GLint y);
GLvoid			(APIENTRY * qglRasterPos2iv)(const GLint *v);
GLvoid			(APIENTRY * qglRasterPos2s)(GLshort x, GLshort y);
GLvoid			(APIENTRY * qglRasterPos2sv)(const GLshort *v);
GLvoid			(APIENTRY * qglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
GLvoid			(APIENTRY * qglRasterPos3dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
GLvoid			(APIENTRY * qglRasterPos3fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglRasterPos3i)(GLint x, GLint y, GLint z);
GLvoid			(APIENTRY * qglRasterPos3iv)(const GLint *v);
GLvoid			(APIENTRY * qglRasterPos3s)(GLshort x, GLshort y, GLshort z);
GLvoid			(APIENTRY * qglRasterPos3sv)(const GLshort *v);
GLvoid			(APIENTRY * qglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLvoid			(APIENTRY * qglRasterPos4dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid			(APIENTRY * qglRasterPos4fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
GLvoid			(APIENTRY * qglRasterPos4iv)(const GLint *v);
GLvoid			(APIENTRY * qglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
GLvoid			(APIENTRY * qglRasterPos4sv)(const GLshort *v);
GLvoid			(APIENTRY * qglReadBuffer)(GLenum mode);
GLvoid			(APIENTRY * qglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GLvoid			(APIENTRY * qglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
GLvoid			(APIENTRY * qglRectdv)(const GLdouble *v1, const GLdouble *v2);
GLvoid			(APIENTRY * qglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
GLvoid			(APIENTRY * qglRectfv)(const GLfloat *v1, const GLfloat *v2);
GLvoid			(APIENTRY * qglRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
GLvoid			(APIENTRY * qglRectiv)(const GLint *v1, const GLint *v2);
GLvoid			(APIENTRY * qglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
GLvoid			(APIENTRY * qglRectsv)(const GLshort *v1, const GLshort *v2);
GLint			(APIENTRY * qglRenderMode)(GLenum mode);
GLvoid			(APIENTRY * qglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
GLvoid			(APIENTRY * qglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GLvoid			(APIENTRY * qglScaled)(GLdouble x, GLdouble y, GLdouble z);
GLvoid			(APIENTRY * qglScalef)(GLfloat x, GLfloat y, GLfloat z);
GLvoid			(APIENTRY * qglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
GLvoid			(APIENTRY * qglSelectBuffer)(GLsizei size, GLuint *buffer);
GLvoid			(APIENTRY * qglShadeModel)(GLenum mode);
GLvoid			(APIENTRY * qglStencilFunc)(GLenum func, GLint ref, GLuint mask);
GLvoid			(APIENTRY * qglStencilMask)(GLuint mask);
GLvoid			(APIENTRY * qglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
GLvoid			(APIENTRY * qglTexCoord1d)(GLdouble s);
GLvoid			(APIENTRY * qglTexCoord1dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglTexCoord1f)(GLfloat s);
GLvoid			(APIENTRY * qglTexCoord1fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglTexCoord1i)(GLint s);
GLvoid			(APIENTRY * qglTexCoord1iv)(const GLint *v);
GLvoid			(APIENTRY * qglTexCoord1s)(GLshort s);
GLvoid			(APIENTRY * qglTexCoord1sv)(const GLshort *v);
GLvoid			(APIENTRY * qglTexCoord2d)(GLdouble s, GLdouble t);
GLvoid			(APIENTRY * qglTexCoord2dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglTexCoord2f)(GLfloat s, GLfloat t);
GLvoid			(APIENTRY * qglTexCoord2fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglTexCoord2i)(GLint s, GLint t);
GLvoid			(APIENTRY * qglTexCoord2iv)(const GLint *v);
GLvoid			(APIENTRY * qglTexCoord2s)(GLshort s, GLshort t);
GLvoid			(APIENTRY * qglTexCoord2sv)(const GLshort *v);
GLvoid			(APIENTRY * qglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
GLvoid			(APIENTRY * qglTexCoord3dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
GLvoid			(APIENTRY * qglTexCoord3fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglTexCoord3i)(GLint s, GLint t, GLint r);
GLvoid			(APIENTRY * qglTexCoord3iv)(const GLint *v);
GLvoid			(APIENTRY * qglTexCoord3s)(GLshort s, GLshort t, GLshort r);
GLvoid			(APIENTRY * qglTexCoord3sv)(const GLshort *v);
GLvoid			(APIENTRY * qglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLvoid			(APIENTRY * qglTexCoord4dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLvoid			(APIENTRY * qglTexCoord4fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
GLvoid			(APIENTRY * qglTexCoord4iv)(const GLint *v);
GLvoid			(APIENTRY * qglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
GLvoid			(APIENTRY * qglTexCoord4sv)(const GLshort *v);
GLvoid			(APIENTRY * qglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglTexEnvi)(GLenum target, GLenum pname, GLint param);
GLvoid			(APIENTRY * qglTexEnviv)(GLenum target, GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglTexGend)(GLenum coord, GLenum pname, GLdouble param);
GLvoid			(APIENTRY * qglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
GLvoid			(APIENTRY * qglTexGenf)(GLenum coord, GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglTexGeni)(GLenum coord, GLenum pname, GLint param);
GLvoid			(APIENTRY * qglTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid			(APIENTRY * qglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid			(APIENTRY * qglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglTexParameteri)(GLenum target, GLenum pname, GLint param);
GLvoid			(APIENTRY * qglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid			(APIENTRY * qglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid			(APIENTRY * qglTranslated)(GLdouble x, GLdouble y, GLdouble z);
GLvoid			(APIENTRY * qglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
GLvoid			(APIENTRY * qglVertex2d)(GLdouble x, GLdouble y);
GLvoid			(APIENTRY * qglVertex2dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglVertex2f)(GLfloat x, GLfloat y);
GLvoid			(APIENTRY * qglVertex2fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglVertex2i)(GLint x, GLint y);
GLvoid			(APIENTRY * qglVertex2iv)(const GLint *v);
GLvoid			(APIENTRY * qglVertex2s)(GLshort x, GLshort y);
GLvoid			(APIENTRY * qglVertex2sv)(const GLshort *v);
GLvoid			(APIENTRY * qglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
GLvoid			(APIENTRY * qglVertex3dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
GLvoid			(APIENTRY * qglVertex3fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglVertex3i)(GLint x, GLint y, GLint z);
GLvoid			(APIENTRY * qglVertex3iv)(const GLint *v);
GLvoid			(APIENTRY * qglVertex3s)(GLshort x, GLshort y, GLshort z);
GLvoid			(APIENTRY * qglVertex3sv)(const GLshort *v);
GLvoid			(APIENTRY * qglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLvoid			(APIENTRY * qglVertex4dv)(const GLdouble *v);
GLvoid			(APIENTRY * qglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid			(APIENTRY * qglVertex4fv)(const GLfloat *v);
GLvoid			(APIENTRY * qglVertex4i)(GLint x, GLint y, GLint z, GLint w);
GLvoid			(APIENTRY * qglVertex4iv)(const GLint *v);
GLvoid			(APIENTRY * qglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
GLvoid			(APIENTRY * qglVertex4sv)(const GLshort *v);
GLvoid			(APIENTRY * qglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

GLvoid			(APIENTRY * qglActiveTextureARB)(GLenum texture);
GLvoid			(APIENTRY * qglClientActiveTextureARB)(GLenum texture);

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
GLvoid *		(APIENTRY * qglMapBufferARB)(GLenum target, GLenum access);
GLboolean		(APIENTRY * qglUnmapBufferARB)(GLenum target);

GLvoid			(APIENTRY * qglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GLvoid			(APIENTRY * qglEnableVertexAttribArrayARB)(GLuint index);
GLvoid			(APIENTRY * qglDisableVertexAttribArrayARB)(GLuint index);
GLvoid			(APIENTRY * qglBindProgramARB)(GLenum target, GLuint program);
GLvoid			(APIENTRY * qglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
GLvoid			(APIENTRY * qglGenProgramsARB)(GLsizei n, GLuint *programs);
GLvoid			(APIENTRY * qglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
GLvoid			(APIENTRY * qglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid			(APIENTRY * qglProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
GLvoid			(APIENTRY * qglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid			(APIENTRY * qglProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
GLvoid			(APIENTRY * qglGetProgramivARB)(GLenum target, GLenum pname, GLint *params);

GLvoid			(APIENTRY * qglDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

GLvoid			(APIENTRY * qglActiveStencilFaceEXT)(GLenum face);

GLvoid			(APIENTRY * qglDepthBoundsEXT)(GLclampd zmin, GLclampd zmax);

BOOL			(WINAPI   * qwglSwapIntervalEXT)(int interval);

GLvoid			(APIENTRY * qglCombinerParameterfvNV)(GLenum pname, const GLfloat *params);
GLvoid			(APIENTRY * qglCombinerParameterfNV)(GLenum pname, GLfloat param);
GLvoid			(APIENTRY * qglCombinerParameterivNV)(GLenum pname, const GLint *params);
GLvoid			(APIENTRY * qglCombinerParameteriNV)(GLenum pname, GLint param);
GLvoid			(APIENTRY * qglCombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
GLvoid			(APIENTRY * qglCombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
GLvoid			(APIENTRY * qglFinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);

GLuint			(APIENTRY * qglGenFragmentShadersATI)(GLuint range);
GLvoid			(APIENTRY * qglBindFragmentShaderATI)(GLuint id);
GLvoid			(APIENTRY * qglDeleteFragmentShaderATI)(GLuint id);
GLvoid			(APIENTRY * qglBeginFragmentShaderATI)(GLvoid);
GLvoid			(APIENTRY * qglEndFragmentShaderATI)(GLvoid);
GLvoid			(APIENTRY * qglPassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle);
GLvoid			(APIENTRY * qglSampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle);
GLvoid			(APIENTRY * qglColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
GLvoid			(APIENTRY * qglColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
GLvoid			(APIENTRY * qglColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
GLvoid			(APIENTRY * qglAlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
GLvoid			(APIENTRY * qglAlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
GLvoid			(APIENTRY * qglAlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
GLvoid			(APIENTRY * qglSetFragmentShaderConstantATI)(GLuint dst, const GLfloat *value);

GLvoid			(APIENTRY * qglStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
GLvoid			(APIENTRY * qglStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);


// =====================================================================


static GLvoid			(APIENTRY * dllAccum)(GLenum op, GLfloat value);
static GLvoid 			(APIENTRY * dllAlphaFunc)(GLenum func, GLclampf ref);
static GLboolean		(APIENTRY * dllAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
static GLvoid 			(APIENTRY * dllArrayElement)(GLint i);
static GLvoid 			(APIENTRY * dllBegin)(GLenum mode);
static GLvoid 			(APIENTRY * dllBindTexture)(GLenum target, GLuint texture);
static GLvoid 			(APIENTRY * dllBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static GLvoid 			(APIENTRY * dllBlendFunc)(GLenum sfactor, GLenum dfactor);
static GLvoid 			(APIENTRY * dllCallList)(GLuint list);
static GLvoid 			(APIENTRY * dllCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
static GLvoid 			(APIENTRY * dllClear)(GLbitfield mask);
static GLvoid 			(APIENTRY * dllClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static GLvoid 			(APIENTRY * dllClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static GLvoid 			(APIENTRY * dllClearDepth)(GLclampd depth);
static GLvoid 			(APIENTRY * dllClearIndex)(GLfloat c);
static GLvoid 			(APIENTRY * dllClearStencil)(GLint s);
static GLvoid 			(APIENTRY * dllClipPlane)(GLenum plane, const GLdouble *equation);
static GLvoid 			(APIENTRY * dllColor3b)(GLbyte red, GLbyte green, GLbyte blue);
static GLvoid 			(APIENTRY * dllColor3bv)(const GLbyte *v);
static GLvoid 			(APIENTRY * dllColor3d)(GLdouble red, GLdouble green, GLdouble blue);
static GLvoid 			(APIENTRY * dllColor3dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllColor3f)(GLfloat red, GLfloat green, GLfloat blue);
static GLvoid 			(APIENTRY * dllColor3fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllColor3i)(GLint red, GLint green, GLint blue);
static GLvoid 			(APIENTRY * dllColor3iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllColor3s)(GLshort red, GLshort green, GLshort blue);
static GLvoid 			(APIENTRY * dllColor3sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
static GLvoid 			(APIENTRY * dllColor3ubv)(const GLubyte *v);
static GLvoid 			(APIENTRY * dllColor3ui)(GLuint red, GLuint green, GLuint blue);
static GLvoid 			(APIENTRY * dllColor3uiv)(const GLuint *v);
static GLvoid 			(APIENTRY * dllColor3us)(GLushort red, GLushort green, GLushort blue);
static GLvoid 			(APIENTRY * dllColor3usv)(const GLushort *v);
static GLvoid 			(APIENTRY * dllColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static GLvoid 			(APIENTRY * dllColor4bv)(const GLbyte *v);
static GLvoid 			(APIENTRY * dllColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static GLvoid 			(APIENTRY * dllColor4dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static GLvoid 			(APIENTRY * dllColor4fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
static GLvoid 			(APIENTRY * dllColor4iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
static GLvoid 			(APIENTRY * dllColor4sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static GLvoid 			(APIENTRY * dllColor4ubv)(const GLubyte *v);
static GLvoid 			(APIENTRY * dllColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
static GLvoid 			(APIENTRY * dllColor4uiv)(const GLuint *v);
static GLvoid 			(APIENTRY * dllColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
static GLvoid 			(APIENTRY * dllColor4usv)(const GLushort *v);
static GLvoid 			(APIENTRY * dllColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static GLvoid 			(APIENTRY * dllColorMaterial)(GLenum face, GLenum mode);
static GLvoid 			(APIENTRY * dllColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static GLvoid 			(APIENTRY * dllCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
static GLvoid 			(APIENTRY * dllCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static GLvoid 			(APIENTRY * dllCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static GLvoid 			(APIENTRY * dllCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static GLvoid 			(APIENTRY * dllCullFace)(GLenum mode);
static GLvoid 			(APIENTRY * dllDeleteLists)(GLuint list, GLsizei range);
static GLvoid 			(APIENTRY * dllDeleteTextures)(GLsizei n, const GLuint *textures);
static GLvoid 			(APIENTRY * dllDepthFunc)(GLenum func);
static GLvoid 			(APIENTRY * dllDepthMask)(GLboolean flag);
static GLvoid 			(APIENTRY * dllDepthRange)(GLclampd zNear, GLclampd zFar);
static GLvoid 			(APIENTRY * dllDisable)(GLenum cap);
static GLvoid 			(APIENTRY * dllDisableClientState)(GLenum array);
static GLvoid 			(APIENTRY * dllDrawArrays)(GLenum mode, GLint first, GLsizei count);
static GLvoid 			(APIENTRY * dllDrawBuffer)(GLenum mode);
static GLvoid 			(APIENTRY * dllDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static GLvoid 			(APIENTRY * dllDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid 			(APIENTRY * dllEdgeFlag)(GLboolean flag);
static GLvoid 			(APIENTRY * dllEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllEdgeFlagv)(const GLboolean *flag);
static GLvoid 			(APIENTRY * dllEnable)(GLenum cap);
static GLvoid 			(APIENTRY * dllEnableClientState)(GLenum array);
static GLvoid 			(APIENTRY * dllEnd)(GLvoid);
static GLvoid 			(APIENTRY * dllEndList)(GLvoid);
static GLvoid 			(APIENTRY * dllEvalCoord1d)(GLdouble u);
static GLvoid 			(APIENTRY * dllEvalCoord1dv)(const GLdouble *u);
static GLvoid 			(APIENTRY * dllEvalCoord1f)(GLfloat u);
static GLvoid 			(APIENTRY * dllEvalCoord1fv)(const GLfloat *u);
static GLvoid 			(APIENTRY * dllEvalCoord2d)(GLdouble u, GLdouble v);
static GLvoid 			(APIENTRY * dllEvalCoord2dv)(const GLdouble *u);
static GLvoid 			(APIENTRY * dllEvalCoord2f)(GLfloat u, GLfloat v);
static GLvoid 			(APIENTRY * dllEvalCoord2fv)(const GLfloat *u);
static GLvoid 			(APIENTRY * dllEvalMesh1)(GLenum mode, GLint i1, GLint i2);
static GLvoid 			(APIENTRY * dllEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static GLvoid 			(APIENTRY * dllEvalPoint1)(GLint i);
static GLvoid 			(APIENTRY * dllEvalPoint2)(GLint i, GLint j);
static GLvoid 			(APIENTRY * dllFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
static GLvoid 			(APIENTRY * dllFinish)(GLvoid);
static GLvoid 			(APIENTRY * dllFlush)(GLvoid);
static GLvoid 			(APIENTRY * dllFogf)(GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllFogfv)(GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllFogi)(GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllFogiv)(GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllFrontFace)(GLenum mode);
static GLvoid 			(APIENTRY * dllFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static GLuint			(APIENTRY * dllGenLists)(GLsizei range);
static GLvoid 			(APIENTRY * dllGenTextures)(GLsizei n, GLuint *textures);
static GLvoid 			(APIENTRY * dllGetBooleanv)(GLenum pname, GLboolean *params);
static GLvoid 			(APIENTRY * dllGetClipPlane)(GLenum plane, GLdouble *equation);
static GLvoid 			(APIENTRY * dllGetDoublev)(GLenum pname, GLdouble *params);
static GLenum			(APIENTRY * dllGetError)(GLvoid);
static GLvoid 			(APIENTRY * dllGetFloatv)(GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetIntegerv)(GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetLightiv)(GLenum light, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetMapdv)(GLenum target, GLenum query, GLdouble *v);
static GLvoid 			(APIENTRY * dllGetMapfv)(GLenum target, GLenum query, GLfloat *v);
static GLvoid 			(APIENTRY * dllGetMapiv)(GLenum target, GLenum query, GLint *v);
static GLvoid 			(APIENTRY * dllGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetPixelMapfv)(GLenum map, GLfloat *values);
static GLvoid 			(APIENTRY * dllGetPixelMapuiv)(GLenum map, GLuint *values);
static GLvoid 			(APIENTRY * dllGetPixelMapusv)(GLenum map, GLushort *values);
static GLvoid 			(APIENTRY * dllGetPointerv)(GLenum pname, GLvoid* *params);
static GLvoid 			(APIENTRY * dllGetPolygonStipple)(GLubyte *mask);
static const GLubyte *	(APIENTRY * dllGetString)(GLenum name);
static GLvoid 			(APIENTRY * dllGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
static GLvoid 			(APIENTRY * dllGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static GLvoid 			(APIENTRY * dllGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
static GLvoid 			(APIENTRY * dllGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
static GLvoid 			(APIENTRY * dllHint)(GLenum target, GLenum mode);
static GLvoid 			(APIENTRY * dllIndexMask)(GLuint mask);
static GLvoid 			(APIENTRY * dllIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllIndexd)(GLdouble c);
static GLvoid 			(APIENTRY * dllIndexdv)(const GLdouble *c);
static GLvoid 			(APIENTRY * dllIndexf)(GLfloat c);
static GLvoid 			(APIENTRY * dllIndexfv)(const GLfloat *c);
static GLvoid 			(APIENTRY * dllIndexi)(GLint c);
static GLvoid 			(APIENTRY * dllIndexiv)(const GLint *c);
static GLvoid 			(APIENTRY * dllIndexs)(GLshort c);
static GLvoid 			(APIENTRY * dllIndexsv)(const GLshort *c);
static GLvoid 			(APIENTRY * dllIndexub)(GLubyte c);
static GLvoid 			(APIENTRY * dllIndexubv)(const GLubyte *c);
static GLvoid 			(APIENTRY * dllInitNames)(GLvoid);
static GLvoid 			(APIENTRY * dllInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
static GLboolean		(APIENTRY * dllIsEnabled)(GLenum cap);
static GLboolean		(APIENTRY * dllIsList)(GLuint list);
static GLboolean		(APIENTRY * dllIsTexture)(GLuint texture);
static GLvoid 			(APIENTRY * dllLightModelf)(GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllLightModelfv)(GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllLightModeli)(GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllLightModeliv)(GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllLightf)(GLenum light, GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllLightfv)(GLenum light, GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllLighti)(GLenum light, GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllLightiv)(GLenum light, GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllLineStipple)(GLint factor, GLushort pattern);
static GLvoid 			(APIENTRY * dllLineWidth)(GLfloat width);
static GLvoid 			(APIENTRY * dllListBase)(GLuint base);
static GLvoid 			(APIENTRY * dllLoadIdentity)(GLvoid);
static GLvoid 			(APIENTRY * dllLoadMatrixd)(const GLdouble *m);
static GLvoid 			(APIENTRY * dllLoadMatrixf)(const GLfloat *m);
static GLvoid 			(APIENTRY * dllLoadName)(GLuint name);
static GLvoid 			(APIENTRY * dllLogicOp)(GLenum opcode);
static GLvoid 			(APIENTRY * dllMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static GLvoid 			(APIENTRY * dllMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static GLvoid 			(APIENTRY * dllMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static GLvoid 			(APIENTRY * dllMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static GLvoid 			(APIENTRY * dllMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
static GLvoid 			(APIENTRY * dllMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
static GLvoid 			(APIENTRY * dllMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static GLvoid 			(APIENTRY * dllMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static GLvoid 			(APIENTRY * dllMaterialf)(GLenum face, GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllMateriali)(GLenum face, GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllMaterialiv)(GLenum face, GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllMatrixMode)(GLenum mode);
static GLvoid 			(APIENTRY * dllMultMatrixd)(const GLdouble *m);
static GLvoid 			(APIENTRY * dllMultMatrixf)(const GLfloat *m);
static GLvoid 			(APIENTRY * dllNewList)(GLuint list, GLenum mode);
static GLvoid 			(APIENTRY * dllNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
static GLvoid 			(APIENTRY * dllNormal3bv)(const GLbyte *v);
static GLvoid 			(APIENTRY * dllNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
static GLvoid 			(APIENTRY * dllNormal3dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
static GLvoid 			(APIENTRY * dllNormal3fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllNormal3i)(GLint nx, GLint ny, GLint nz);
static GLvoid 			(APIENTRY * dllNormal3iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllNormal3s)(GLshort nx, GLshort ny, GLshort nz);
static GLvoid 			(APIENTRY * dllNormal3sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static GLvoid 			(APIENTRY * dllPassThrough)(GLfloat token);
static GLvoid 			(APIENTRY * dllPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
static GLvoid 			(APIENTRY * dllPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
static GLvoid 			(APIENTRY * dllPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
static GLvoid 			(APIENTRY * dllPixelStoref)(GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllPixelStorei)(GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllPixelTransferf)(GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllPixelTransferi)(GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllPixelZoom)(GLfloat xfactor, GLfloat yfactor);
static GLvoid 			(APIENTRY * dllPointSize)(GLfloat size);
static GLvoid 			(APIENTRY * dllPolygonMode)(GLenum face, GLenum mode);
static GLvoid 			(APIENTRY * dllPolygonOffset)(GLfloat factor, GLfloat units);
static GLvoid 			(APIENTRY * dllPolygonStipple)(const GLubyte *mask);
static GLvoid 			(APIENTRY * dllPopAttrib)(GLvoid);
static GLvoid 			(APIENTRY * dllPopClientAttrib)(GLvoid);
static GLvoid 			(APIENTRY * dllPopMatrix)(GLvoid);
static GLvoid 			(APIENTRY * dllPopName)(GLvoid);
static GLvoid 			(APIENTRY * dllPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
static GLvoid 			(APIENTRY * dllPushAttrib)(GLbitfield mask);
static GLvoid 			(APIENTRY * dllPushClientAttrib)(GLbitfield mask);
static GLvoid 			(APIENTRY * dllPushMatrix)(GLvoid);
static GLvoid 			(APIENTRY * dllPushName)(GLuint name);
static GLvoid 			(APIENTRY * dllRasterPos2d)(GLdouble x, GLdouble y);
static GLvoid 			(APIENTRY * dllRasterPos2dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllRasterPos2f)(GLfloat x, GLfloat y);
static GLvoid 			(APIENTRY * dllRasterPos2fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllRasterPos2i)(GLint x, GLint y);
static GLvoid 			(APIENTRY * dllRasterPos2iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllRasterPos2s)(GLshort x, GLshort y);
static GLvoid 			(APIENTRY * dllRasterPos2sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid 			(APIENTRY * dllRasterPos3dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid 			(APIENTRY * dllRasterPos3fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllRasterPos3i)(GLint x, GLint y, GLint z);
static GLvoid 			(APIENTRY * dllRasterPos3iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllRasterPos3s)(GLshort x, GLshort y, GLshort z);
static GLvoid 			(APIENTRY * dllRasterPos3sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static GLvoid 			(APIENTRY * dllRasterPos4dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid 			(APIENTRY * dllRasterPos4fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
static GLvoid 			(APIENTRY * dllRasterPos4iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
static GLvoid 			(APIENTRY * dllRasterPos4sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllReadBuffer)(GLenum mode);
static GLvoid 			(APIENTRY * dllReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static GLvoid 			(APIENTRY * dllRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static GLvoid 			(APIENTRY * dllRectdv)(const GLdouble *v1, const GLdouble *v2);
static GLvoid 			(APIENTRY * dllRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static GLvoid 			(APIENTRY * dllRectfv)(const GLfloat *v1, const GLfloat *v2);
static GLvoid 			(APIENTRY * dllRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
static GLvoid 			(APIENTRY * dllRectiv)(const GLint *v1, const GLint *v2);
static GLvoid 			(APIENTRY * dllRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static GLvoid 			(APIENTRY * dllRectsv)(const GLshort *v1, const GLshort *v2);
static GLint			(APIENTRY * dllRenderMode)(GLenum mode);
static GLvoid 			(APIENTRY * dllRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static GLvoid 			(APIENTRY * dllRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static GLvoid 			(APIENTRY * dllScaled)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid 			(APIENTRY * dllScalef)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid 			(APIENTRY * dllScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
static GLvoid 			(APIENTRY * dllSelectBuffer)(GLsizei size, GLuint *buffer);
static GLvoid 			(APIENTRY * dllShadeModel)(GLenum mode);
static GLvoid 			(APIENTRY * dllStencilFunc)(GLenum func, GLint ref, GLuint mask);
static GLvoid 			(APIENTRY * dllStencilMask)(GLuint mask);
static GLvoid 			(APIENTRY * dllStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
static GLvoid 			(APIENTRY * dllTexCoord1d)(GLdouble s);
static GLvoid 			(APIENTRY * dllTexCoord1dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllTexCoord1f)(GLfloat s);
static GLvoid 			(APIENTRY * dllTexCoord1fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllTexCoord1i)(GLint s);
static GLvoid 			(APIENTRY * dllTexCoord1iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllTexCoord1s)(GLshort s);
static GLvoid 			(APIENTRY * dllTexCoord1sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllTexCoord2d)(GLdouble s, GLdouble t);
static GLvoid 			(APIENTRY * dllTexCoord2dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllTexCoord2f)(GLfloat s, GLfloat t);
static GLvoid 			(APIENTRY * dllTexCoord2fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllTexCoord2i)(GLint s, GLint t);
static GLvoid 			(APIENTRY * dllTexCoord2iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllTexCoord2s)(GLshort s, GLshort t);
static GLvoid 			(APIENTRY * dllTexCoord2sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
static GLvoid 			(APIENTRY * dllTexCoord3dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
static GLvoid 			(APIENTRY * dllTexCoord3fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllTexCoord3i)(GLint s, GLint t, GLint r);
static GLvoid 			(APIENTRY * dllTexCoord3iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllTexCoord3s)(GLshort s, GLshort t, GLshort r);
static GLvoid 			(APIENTRY * dllTexCoord3sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static GLvoid 			(APIENTRY * dllTexCoord4dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static GLvoid 			(APIENTRY * dllTexCoord4fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
static GLvoid 			(APIENTRY * dllTexCoord4iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
static GLvoid 			(APIENTRY * dllTexCoord4sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllTexEnvf)(GLenum target, GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllTexEnvi)(GLenum target, GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllTexEnviv)(GLenum target, GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllTexGend)(GLenum coord, GLenum pname, GLdouble param);
static GLvoid 			(APIENTRY * dllTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
static GLvoid 			(APIENTRY * dllTexGenf)(GLenum coord, GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllTexGeni)(GLenum coord, GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid 			(APIENTRY * dllTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid 			(APIENTRY * dllTexParameterf)(GLenum target, GLenum pname, GLfloat param);
static GLvoid 			(APIENTRY * dllTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
static GLvoid 			(APIENTRY * dllTexParameteri)(GLenum target, GLenum pname, GLint param);
static GLvoid 			(APIENTRY * dllTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
static GLvoid 			(APIENTRY * dllTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid 			(APIENTRY * dllTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid 			(APIENTRY * dllTranslated)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid 			(APIENTRY * dllTranslatef)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid 			(APIENTRY * dllVertex2d)(GLdouble x, GLdouble y);
static GLvoid 			(APIENTRY * dllVertex2dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllVertex2f)(GLfloat x, GLfloat y);
static GLvoid 			(APIENTRY * dllVertex2fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllVertex2i)(GLint x, GLint y);
static GLvoid 			(APIENTRY * dllVertex2iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllVertex2s)(GLshort x, GLshort y);
static GLvoid 			(APIENTRY * dllVertex2sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllVertex3d)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid 			(APIENTRY * dllVertex3dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllVertex3f)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid 			(APIENTRY * dllVertex3fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllVertex3i)(GLint x, GLint y, GLint z);
static GLvoid 			(APIENTRY * dllVertex3iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllVertex3s)(GLshort x, GLshort y, GLshort z);
static GLvoid 			(APIENTRY * dllVertex3sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static GLvoid 			(APIENTRY * dllVertex4dv)(const GLdouble *v);
static GLvoid 			(APIENTRY * dllVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid 			(APIENTRY * dllVertex4fv)(const GLfloat *v);
static GLvoid 			(APIENTRY * dllVertex4i)(GLint x, GLint y, GLint z, GLint w);
static GLvoid 			(APIENTRY * dllVertex4iv)(const GLint *v);
static GLvoid 			(APIENTRY * dllVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
static GLvoid 			(APIENTRY * dllVertex4sv)(const GLshort *v);
static GLvoid 			(APIENTRY * dllVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid 			(APIENTRY * dllViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

static GLvoid			(APIENTRY * dllActiveTextureARB)(GLenum texture);
static GLvoid			(APIENTRY * dllClientActiveTextureARB)(GLenum texture);

static GLvoid			(APIENTRY * dllBindBufferARB)(GLenum target, GLuint buffer);
static GLvoid			(APIENTRY * dllDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
static GLvoid			(APIENTRY * dllGenBuffersARB)(GLsizei n, GLuint *buffers);
static GLvoid			(APIENTRY * dllBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
static GLvoid			(APIENTRY * dllBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
static GLvoid *			(APIENTRY * dllMapBufferARB)(GLenum target, GLenum access);
static GLboolean		(APIENTRY * dllUnmapBufferARB)(GLenum target);

static GLvoid			(APIENTRY * dllVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllEnableVertexAttribArrayARB)(GLuint index);
static GLvoid			(APIENTRY * dllDisableVertexAttribArrayARB)(GLuint index);
static GLvoid			(APIENTRY * dllBindProgramARB)(GLenum target, GLuint program);
static GLvoid			(APIENTRY * dllDeleteProgramsARB)(GLsizei n, const GLuint *programs);
static GLvoid			(APIENTRY * dllGenProgramsARB)(GLsizei n, GLuint *programs);
static GLvoid			(APIENTRY * dllProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
static GLvoid			(APIENTRY * dllProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid			(APIENTRY * dllProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
static GLvoid			(APIENTRY * dllProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid			(APIENTRY * dllProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
static GLvoid			(APIENTRY * dllGetProgramivARB)(GLenum target, GLenum pname, GLint *params);

static GLvoid			(APIENTRY * dllDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

static GLvoid			(APIENTRY * dllActiveStencilFaceEXT)(GLenum face);

static GLvoid			(APIENTRY * dllDepthBoundsEXT)(GLclampd zmin, GLclampd zmax);

static BOOL				(WINAPI   * dllSwapIntervalEXT)(int interval);

static GLvoid			(APIENTRY * dllCombinerParameterfvNV)(GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllCombinerParameterfNV)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllCombinerParameterivNV)(GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllCombinerParameteriNV)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllCombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
static GLvoid			(APIENTRY * dllCombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
static GLvoid			(APIENTRY * dllFinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);

static GLuint			(APIENTRY * dllGenFragmentShadersATI)(GLuint range);
static GLvoid			(APIENTRY * dllBindFragmentShaderATI)(GLuint id);
static GLvoid			(APIENTRY * dllDeleteFragmentShaderATI)(GLuint id);
static GLvoid			(APIENTRY * dllBeginFragmentShaderATI)(GLvoid);
static GLvoid			(APIENTRY * dllEndFragmentShaderATI)(GLvoid);
static GLvoid			(APIENTRY * dllPassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle);
static GLvoid			(APIENTRY * dllSampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle);
static GLvoid			(APIENTRY * dllColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
static GLvoid			(APIENTRY * dllColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
static GLvoid			(APIENTRY * dllColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
static GLvoid			(APIENTRY * dllAlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
static GLvoid			(APIENTRY * dllAlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
static GLvoid			(APIENTRY * dllAlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
static GLvoid			(APIENTRY * dllSetFragmentShaderConstantATI)(GLuint dst, const GLfloat *value);

static GLvoid			(APIENTRY * dllStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
static GLvoid			(APIENTRY * dllStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);


// =====================================================================


static GLvoid APIENTRY logAccum (GLenum op, GLfloat value){

	fprintf(glwState.logFile, "glAccum\n");
	dllAccum(op, value);
}

static GLvoid APIENTRY logAlphaFunc (GLenum func, GLclampf ref){

	char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";			break;
	case GL_LESS:		f = "GL_LESS";			break;
	case GL_EQUAL:		f = "GL_EQUAL";			break;
	case GL_LEQUAL:		f = "GL_LEQUAL";		break;
	case GL_GREATER:	f = "GL_GREATER";		break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";		break;
	case GL_GEQUAL:		f = "GL_GEQUAL";		break;
	case GL_ALWAYS:		f = "GL_ALWAYS";		break;
	default:			f = va("0x%X", func);	break;
	}

	fprintf(glwState.logFile, "glAlphaFunc( %s, %g )\n", f, ref);
	dllAlphaFunc(func, ref);
}

static GLboolean APIENTRY logAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences){

	fprintf(glwState.logFile, "glAreTexturesResident\n");
	return dllAreTexturesResident(n, textures, residences);
}

static GLvoid APIENTRY logArrayElement (GLint i){

	fprintf(glwState.logFile, "glArrayElement( %i )\n", i);
	dllArrayElement(i);
}

static GLvoid APIENTRY logBegin (GLenum mode){

	char	*m;

	switch (mode){
	case GL_POINTS:			m = "GL_POINTS";			break;
	case GL_LINES:			m = "GL_LINES";				break;
	case GL_LINE_STRIP:		m = "GL_LINE_STRIP";		break;
	case GL_LINE_LOOP:		m = "GL_LINE_LOOP";			break;
	case GL_TRIANGLES:		m = "GL_TRIANGLES";			break;
	case GL_TRIANGLE_STRIP: m = "GL_TRIANGLE_STRIP";	break;
	case GL_TRIANGLE_FAN:	m = "GL_TRIANGLE_FAN";		break;
	case GL_QUADS:			m = "GL_QUADS";				break;
	case GL_QUAD_STRIP:		m = "GL_QUAD_STRIP";		break;
	case GL_POLYGON:		m = "GL_POLYGON";			break;
	default:				m = va("0x%X", mode);		break;
	}

	fprintf(glwState.logFile, "glBegin( %s )\n%d\n", m, mode); // CHANGED: Added %d to eliminate gcc warning
	dllBegin(mode);
}

static GLvoid APIENTRY logBindTexture (GLenum target, GLuint texture){

	char	*t;

	switch (target){
	case GL_TEXTURE_1D:				t = "GL_TEXTURE_1D";		break;
	case GL_TEXTURE_2D:				t = "GL_TEXTURE_2D";		break;
	case GL_TEXTURE_3D:				t = "GL_TEXTURE_3D";		break;
	case GL_TEXTURE_CUBE_MAP_ARB:	t = "GL_TEXTURE_CUBE_MAP";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glBindTexture( %s, %u )\n", t, texture);
	dllBindTexture(target, texture);
}

static GLvoid APIENTRY logBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap){

	fprintf(glwState.logFile, "glBitmap\n");
	dllBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}

static GLvoid APIENTRY logBlendFunc (GLenum sfactor, GLenum dfactor){

	char	*s, *d;

	switch (sfactor){
	case GL_ZERO:					s = "GL_ZERO";					break;
	case GL_ONE:					s = "GL_ONE";					break;
	case GL_DST_COLOR:				s = "GL_DST_COLOR";				break;
	case GL_ONE_MINUS_DST_COLOR:	s = "GL_ONE_MINUS_DST_COLOR";	break;
	case GL_SRC_ALPHA:				s = "GL_SRC_ALPHA";				break;
	case GL_ONE_MINUS_SRC_ALPHA:	s = "GL_ONE_MINUS_SRC_ALPHA";	break;
	case GL_DST_ALPHA:				s = "GL_DST_ALPHA";				break;
	case GL_ONE_MINUS_DST_ALPHA:	s = "GL_ONE_MINUS_DST_ALPHA";	break;
	case GL_SRC_ALPHA_SATURATE:		s = "GL_SRC_ALPHA_SATURATE";	break;
	default:						s = va("0x%X", sfactor);		break;
	}

	switch (dfactor){
	case GL_ZERO:					d = "GL_ZERO";					break;
	case GL_ONE:					d = "GL_ONE";					break;
	case GL_SRC_COLOR:				d = "GL_SRC_COLOR";				break;
	case GL_ONE_MINUS_SRC_COLOR:	d = "GL_ONE_MINUS_SRC_COLOR";	break;
	case GL_SRC_ALPHA:				d = "GL_SRC_ALPHA";				break;
	case GL_ONE_MINUS_SRC_ALPHA:	d = "GL_ONE_MINUS_SRC_ALPHA";	break;
	case GL_DST_ALPHA:				d = "GL_DST_ALPHA";				break;
	case GL_ONE_MINUS_DST_ALPHA:	d = "GL_ONE_MINUS_DST_ALPHA";	break;
	default:						d = va("0x%X", dfactor);		break;
	}

	fprintf(glwState.logFile, "glBlendFunc( %s, %s )\n", s, d);
	dllBlendFunc(sfactor, dfactor);
}

static GLvoid APIENTRY logCallList (GLuint list){

	fprintf(glwState.logFile, "glCallList\n");
	dllCallList(list);
}

static GLvoid APIENTRY logCallLists (GLsizei n, GLenum type, const GLvoid *lists){

	fprintf(glwState.logFile, "glCallLists\n");
	dllCallLists(n, type, lists);
}

static GLvoid APIENTRY logClear (GLbitfield mask){

	fprintf(glwState.logFile, "glClear( ");

	if (mask & GL_ACCUM_BUFFER_BIT)
		fprintf(glwState.logFile, "GL_ACCUM_BUFFER_BIT ");
	if (mask & GL_COLOR_BUFFER_BIT)
		fprintf(glwState.logFile, "GL_COLOR_BUFFER_BIT ");
	if (mask & GL_DEPTH_BUFFER_BIT)
		fprintf(glwState.logFile, "GL_DEPTH_BUFFER_BIT ");
	if (mask & GL_STENCIL_BUFFER_BIT)
		fprintf(glwState.logFile, "GL_STENCIL_BUFFER_BIT ");

	fprintf(glwState.logFile, ")\n");
	dllClear(mask);
}

static GLvoid APIENTRY logClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){

	fprintf(glwState.logFile, "glClearAccum\n");
	dllClearAccum(red, green, blue, alpha);
}

static GLvoid APIENTRY logClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){

	fprintf(glwState.logFile, "glClearColor( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllClearColor(red, green, blue, alpha);
}

static GLvoid APIENTRY logClearDepth (GLclampd depth){

	fprintf(glwState.logFile, "glClearDepth( %g )\n", depth);
	dllClearDepth(depth);
}

static GLvoid APIENTRY logClearIndex (GLfloat c){

	fprintf(glwState.logFile, "glClearIndex\n");
	dllClearIndex(c);
}

static GLvoid APIENTRY logClearStencil (GLint s){

	fprintf(glwState.logFile, "glClearStencil( %i )\n", s);
	dllClearStencil(s);
}

static GLvoid APIENTRY logClipPlane (GLenum plane, const GLdouble *equation){

	fprintf(glwState.logFile, "glClipPlane\n");
	dllClipPlane(plane, equation);
}

static GLvoid APIENTRY logColor3b (GLbyte red, GLbyte green, GLbyte blue){

	fprintf(glwState.logFile, "glColor3b\n");
	dllColor3b(red, green, blue);
}

static GLvoid APIENTRY logColor3bv (const GLbyte *v){

	fprintf(glwState.logFile, "glColor3bv\n");
	dllColor3bv(v);
}

static GLvoid APIENTRY logColor3d (GLdouble red, GLdouble green, GLdouble blue){

	fprintf(glwState.logFile, "glColor3d\n");
	dllColor3d(red, green, blue);
}

static GLvoid APIENTRY logColor3dv (const GLdouble *v){

	fprintf(glwState.logFile, "glColor3dv\n");
	dllColor3dv(v);
}

static GLvoid APIENTRY logColor3f (GLfloat red, GLfloat green, GLfloat blue){

	fprintf(glwState.logFile, "glColor3f\n");
	dllColor3f(red, green, blue);
}

static GLvoid APIENTRY logColor3fv (const GLfloat *v){

	fprintf(glwState.logFile, "glColor3fv\n");
	dllColor3fv(v);
}

static GLvoid APIENTRY logColor3i (GLint red, GLint green, GLint blue){

	fprintf(glwState.logFile, "glColor3i\n");
	dllColor3i(red, green, blue);
}

static GLvoid APIENTRY logColor3iv (const GLint *v){

	fprintf(glwState.logFile, "glColor3iv\n");
	dllColor3iv(v);
}

static GLvoid APIENTRY logColor3s (GLshort red, GLshort green, GLshort blue){

	fprintf(glwState.logFile, "glColor3s\n");
	dllColor3s(red, green, blue);
}

static GLvoid APIENTRY logColor3sv (const GLshort *v){

	fprintf(glwState.logFile, "glColor3sv\n");
	dllColor3sv(v);
}

static GLvoid APIENTRY logColor3ub (GLubyte red, GLubyte green, GLubyte blue){

	fprintf(glwState.logFile, "glColor3ub\n");
	dllColor3ub(red, green, blue);
}

static GLvoid APIENTRY logColor3ubv (const GLubyte *v){

	fprintf(glwState.logFile, "glColor3ubv\n");
	dllColor3ubv(v);
}

static GLvoid APIENTRY logColor3ui (GLuint red, GLuint green, GLuint blue){

	fprintf(glwState.logFile, "glColor3ui\n");
	dllColor3ui(red, green, blue);
}

static GLvoid APIENTRY logColor3uiv (const GLuint *v){

	fprintf(glwState.logFile, "glColor3uiv\n");
	dllColor3uiv(v);
}

static GLvoid APIENTRY logColor3us (GLushort red, GLushort green, GLushort blue){

	fprintf(glwState.logFile, "glColor3us\n");
	dllColor3us(red, green, blue);
}

static GLvoid APIENTRY logColor3usv (const GLushort *v){

	fprintf(glwState.logFile, "glColor3usv\n");
	dllColor3usv(v);
}

static GLvoid APIENTRY logColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha){

	fprintf(glwState.logFile, "glColor4b\n");
	dllColor4b(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4bv (const GLbyte *v){

	fprintf(glwState.logFile, "glColor4bv\n");
	dllColor4bv(v);
}

static GLvoid APIENTRY logColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha){

	fprintf(glwState.logFile, "glColor4d\n");
	dllColor4d(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4dv (const GLdouble *v){

	fprintf(glwState.logFile, "glColor4dv\n");
	dllColor4dv(v);
}

static GLvoid APIENTRY logColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){

	fprintf(glwState.logFile, "glColor4f\n");
	dllColor4f(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4fv (const GLfloat *v){

	fprintf(glwState.logFile, "glColor4fv\n");
	dllColor4fv(v);
}

static GLvoid APIENTRY logColor4i (GLint red, GLint green, GLint blue, GLint alpha){

	fprintf(glwState.logFile, "glColor4i\n");
	dllColor4i(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4iv (const GLint *v){

	fprintf(glwState.logFile, "glColor4iv\n");
	dllColor4iv(v);
}

static GLvoid APIENTRY logColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha){

	fprintf(glwState.logFile, "glColor4s\n");
	dllColor4s(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4sv (const GLshort *v){

	fprintf(glwState.logFile, "glColor4sv\n");
	dllColor4sv(v);
}

static GLvoid APIENTRY logColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha){

	fprintf(glwState.logFile, "glColor4ub\n");
	dllColor4b(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4ubv (const GLubyte *v){

	fprintf(glwState.logFile, "glColor4ubv\n");
	dllColor4ubv(v);
}

static GLvoid APIENTRY logColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha){

	fprintf(glwState.logFile, "glColor4ui\n");
	dllColor4ui(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4uiv (const GLuint *v){

	fprintf(glwState.logFile, "glColor4uiv\n");
	dllColor4uiv(v);
}

static GLvoid APIENTRY logColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha){

	fprintf(glwState.logFile, "glColor4us\n");
	dllColor4us(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4usv (const GLushort *v){

	fprintf(glwState.logFile, "glColor4usv\n");
	dllColor4usv(v);
}

static GLvoid APIENTRY logColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){

	char	*r, *g, *b, *a;

	switch (red){
	case GL_FALSE:		r = "GL_FALSE";			break;
	case GL_TRUE:		r = "GL_TRUE";			break;
	default:			r = va("0x%X", red);	break;
	}

	switch (green){
	case GL_FALSE:		g = "GL_FALSE";			break;
	case GL_TRUE:		g = "GL_TRUE";			break;
	default:			g = va("0x%X", green);	break;
	}

	switch (blue){
	case GL_FALSE:		b = "GL_FALSE";			break;
	case GL_TRUE:		b = "GL_TRUE";			break;
	default:			b = va("0x%X", blue);	break;
	}

	switch (alpha){
	case GL_FALSE:		a = "GL_FALSE";			break;
	case GL_TRUE:		a = "GL_TRUE";			break;
	default:			a = va("0x%X", alpha);	break;
	}

	fprintf(glwState.logFile, "glColorMask( %s, %s, %s, %s )\n", r, g, b, a);
	dllColorMask(red, green, blue, alpha);
}

static GLvoid APIENTRY logColorMaterial (GLenum face, GLenum mode){

	fprintf(glwState.logFile, "glColorMaterial\n");
	dllColorMaterial(face, mode);
}

static GLvoid APIENTRY logColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glColorPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllColorPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type){

	fprintf(glwState.logFile, "glCopyPixels\n");
	dllCopyPixels(x, y, width, height, type);
}

static GLvoid APIENTRY logCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border){

	char	*t, *i;

	switch (target){
	case GL_TEXTURE_1D:						t = "GL_TEXTURE_1D";					break;
	default:								t = va("0x%X", target);					break;
	}

	switch (internalFormat){
	case GL_ALPHA:							i = "GL_ALPHA";							break;
	case GL_ALPHA4:							i = "GL_ALPHA4";						break;
	case GL_ALPHA8:							i = "GL_ALPHA8";						break;
	case GL_ALPHA12:						i = "GL_ALPHA12";						break;
	case GL_ALPHA16:						i = "GL_ALPHA16";						break;
	case GL_LUMINANCE:						i = "GL_LUMINANCE";						break;
	case GL_LUMINANCE4:						i = "GL_LUMINANCE4";					break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";					break;
	case GL_LUMINANCE12:					i = "GL_LUMINANCE12";					break;
	case GL_LUMINANCE16:					i = "GL_LUMINANCE16";					break;
	case GL_LUMINANCE_ALPHA:				i = "GL_LUMINANCE_ALPHA";				break;
	case GL_LUMINANCE4_ALPHA4:				i = "GL_LUMINANCE4_ALPHA4";				break;
	case GL_LUMINANCE6_ALPHA2:				i = "GL_LUMINANCE6_ALPHA2";				break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";				break;
	case GL_LUMINANCE12_ALPHA4:				i = "GL_LUMINANCE12_ALPHA4";			break;
	case GL_LUMINANCE12_ALPHA12:			i = "GL_LUMINANCE12_ALPHA12";			break;
	case GL_LUMINANCE16_ALPHA16:			i = "GL_LUMINANCE16_ALPHA16";			break;
	case GL_INTENSITY:						i = "GL_INTENSITY";						break;
	case GL_INTENSITY4:						i = "GL_INTENSITY4";					break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";					break;
	case GL_INTENSITY12:					i = "GL_INTENSITY12";					break;
	case GL_INTENSITY16:					i = "GL_INTENSITY16";					break;
	case GL_R3_G3_B2:						i = "GL_R3_G3_B2";						break;
	case GL_RGB:							i = "GL_RGB";							break;
	case GL_RGB4:							i = "GL_RGB4";							break;
	case GL_RGB5:							i = "GL_RGB5";							break;
	case GL_RGB8:							i = "GL_RGB8";							break;
	case GL_RGB10:							i = "GL_RGB10";							break;
	case GL_RGB12:							i = "GL_RGB12";							break;
	case GL_RGB16:							i = "GL_RGB16";							break;
	case GL_RGBA:							i = "GL_RGBA";							break;
	case GL_RGBA2:							i = "GL_RGBA2";							break;
	case GL_RGBA4:							i = "GL_RGBA4";							break;
	case GL_RGB5_A1:						i = "GL_RGB5_A1";						break;
	case GL_RGBA8:							i = "GL_RGBA8";							break;
	case GL_RGB10_A2:						i = "GL_RGB10_A2";						break;
	case GL_RGBA12:							i = "GL_RGBA12";						break;
	case GL_RGBA16:							i = "GL_RGBA16";						break;
	case GL_COMPRESSED_ALPHA_ARB:			i = "GL_COMPRESSED_ALPHA";				break;
	case GL_COMPRESSED_RGB_ARB:				i = "GL_COMPRESSED_RGB";				break;
	case GL_COMPRESSED_RGBA_ARB:			i = "GL_COMPRESSED_RGBA";				break;
	case GL_COMPRESSED_LUMINANCE_ARB:		i = "GL_COMPRESSED_LUMINANCE";			break;
	case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:	i = "GL_COMPRESSED_LUMINANCE_ALPHA";	break;
	case GL_COMPRESSED_INTENSITY_ARB:		i = "GL_COMPRESSED_INTENSITY";			break;
	default:								i = va("0x%X", internalFormat);			break;
	}

	fprintf(glwState.logFile, "glCopyTexImage1D( %s, %i, %s, %i, %i, %i, %i )\n", t, level, i, x, y, width, border);
	dllCopyTexImage1D(target, level, internalFormat, x, y, width, border);
}

static GLvoid APIENTRY logCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){

	char	*t, *i;

	switch (target){
	case GL_TEXTURE_2D:							t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	default:									t = va("0x%X", target);					break;
	}

	switch (internalFormat){
	case GL_ALPHA:								i = "GL_ALPHA";							break;
	case GL_ALPHA4:								i = "GL_ALPHA4";						break;
	case GL_ALPHA8:								i = "GL_ALPHA8";						break;
	case GL_ALPHA12:							i = "GL_ALPHA12";						break;
	case GL_ALPHA16:							i = "GL_ALPHA16";						break;
	case GL_LUMINANCE:							i = "GL_LUMINANCE";						break;
	case GL_LUMINANCE4:							i = "GL_LUMINANCE4";					break;
	case GL_LUMINANCE8:							i = "GL_LUMINANCE8";					break;
	case GL_LUMINANCE12:						i = "GL_LUMINANCE12";					break;
	case GL_LUMINANCE16:						i = "GL_LUMINANCE16";					break;
	case GL_LUMINANCE_ALPHA:					i = "GL_LUMINANCE_ALPHA";				break;
	case GL_LUMINANCE4_ALPHA4:					i = "GL_LUMINANCE4_ALPHA4";				break;
	case GL_LUMINANCE6_ALPHA2:					i = "GL_LUMINANCE6_ALPHA2";				break;
	case GL_LUMINANCE8_ALPHA8:					i = "GL_LUMINANCE8_ALPHA8";				break;
	case GL_LUMINANCE12_ALPHA4:					i = "GL_LUMINANCE12_ALPHA4";			break;
	case GL_LUMINANCE12_ALPHA12:				i = "GL_LUMINANCE12_ALPHA12";			break;
	case GL_LUMINANCE16_ALPHA16:				i = "GL_LUMINANCE16_ALPHA16";			break;
	case GL_INTENSITY:							i = "GL_INTENSITY";						break;
	case GL_INTENSITY4:							i = "GL_INTENSITY4";					break;
	case GL_INTENSITY8:							i = "GL_INTENSITY8";					break;
	case GL_INTENSITY12:						i = "GL_INTENSITY12";					break;
	case GL_INTENSITY16:						i = "GL_INTENSITY16";					break;
	case GL_R3_G3_B2:							i = "GL_R3_G3_B2";						break;
	case GL_RGB:								i = "GL_RGB";							break;
	case GL_RGB4:								i = "GL_RGB4";							break;
	case GL_RGB5:								i = "GL_RGB5";							break;
	case GL_RGB8:								i = "GL_RGB8";							break;
	case GL_RGB10:								i = "GL_RGB10";							break;
	case GL_RGB12:								i = "GL_RGB12";							break;
	case GL_RGB16:								i = "GL_RGB16";							break;
	case GL_RGBA:								i = "GL_RGBA";							break;
	case GL_RGBA2:								i = "GL_RGBA2";							break;
	case GL_RGBA4:								i = "GL_RGBA4";							break;
	case GL_RGB5_A1:							i = "GL_RGB5_A1";						break;
	case GL_RGBA8:								i = "GL_RGBA8";							break;
	case GL_RGB10_A2:							i = "GL_RGB10_A2";						break;
	case GL_RGBA12:								i = "GL_RGBA12";						break;
	case GL_RGBA16:								i = "GL_RGBA16";						break;
	case GL_COMPRESSED_ALPHA_ARB:				i = "GL_COMPRESSED_ALPHA";				break;
	case GL_COMPRESSED_RGB_ARB:					i = "GL_COMPRESSED_RGB";				break;
	case GL_COMPRESSED_RGBA_ARB:				i = "GL_COMPRESSED_RGBA";				break;
	case GL_COMPRESSED_LUMINANCE_ARB:			i = "GL_COMPRESSED_LUMINANCE";			break;
	case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";	break;
	case GL_COMPRESSED_INTENSITY_ARB:			i = "GL_COMPRESSED_INTENSITY";			break;
	default:									i = va("0x%X", internalFormat);			break;
	}

	fprintf(glwState.logFile, "glCopyTexImage2D( %s, %i, %s, %i, %i, %i, %i, %i )\n", t, level, i, x, y, width, height, border);
	dllCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}

static GLvoid APIENTRY logCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width){

	char	*t;

	switch (target){
	case GL_TEXTURE_1D:	t = "GL_TEXTURE_1D";	break;
	default:			t = va("0x%X", target);	break;
	}

	fprintf(glwState.logFile, "glCopyTexSubImage1D( %s, %i, %i, %i, %i, %i)\n", t, level, xoffset, x, y, width);
	dllCopyTexSubImage1D(target, level, xoffset, x, y, width);
}

static GLvoid APIENTRY logCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){

	char	*t;

	switch (target){
	case GL_TEXTURE_2D:							t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	default:									t = va("0x%X", target);					break;
	}

	fprintf(glwState.logFile, "glCopyTexSubImage2D( %s, %i, %i, %i, %i, %i, %i, %i )\n", t, level, xoffset, yoffset, x, y, width, height);
	dllCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

static GLvoid APIENTRY logCullFace (GLenum mode){

	char	*m;

	switch (mode){
	case GL_FRONT:			m = "GL_FRONT";				break;
	case GL_BACK:			m = "GL_BACK";				break;
	case GL_FRONT_AND_BACK:	m = "GL_FRONT_AND_BACK";	break;
	default:				m = va("0x%X", mode);		break;
	}

	fprintf(glwState.logFile, "glCullFace( %s )\n", m);
	dllCullFace(mode);
}

static GLvoid APIENTRY logDeleteLists (GLuint list, GLsizei range){

	fprintf(glwState.logFile, "glDeleteLists\n");
	dllDeleteLists(list, range);
}

static GLvoid APIENTRY logDeleteTextures (GLsizei n, const GLuint *textures){

	fprintf(glwState.logFile, "glDeleteTextures\n");
	dllDeleteTextures(n, textures);
}

static GLvoid APIENTRY logDepthFunc (GLenum func){

	char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";			break;
	case GL_LESS:		f = "GL_LESS";			break;
	case GL_EQUAL:		f = "GL_EQUAL";			break;
	case GL_LEQUAL:		f = "GL_LEQUAL";		break;
	case GL_GREATER:	f = "GL_GREATER";		break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";		break;
	case GL_GEQUAL:		f = "GL_GEQUAL";		break;
	case GL_ALWAYS:		f = "GL_ALWAYS";		break;
	default:			f = va("0x%X", func);	break;
	}

	fprintf(glwState.logFile, "glDepthFunc( %s )\n", f);
	dllDepthFunc(func);
}

static GLvoid APIENTRY logDepthMask (GLboolean flag){

	char	*f;

	switch (flag){
	case GL_FALSE:	f = "GL_FALSE";			break;
	case GL_TRUE:	f = "GL_TRUE";			break;
	default:		f = va("0x%X", flag);	break;
	}

	fprintf(glwState.logFile, "glDepthMask( %s )\n", f);
	dllDepthMask(flag);
}

static GLvoid APIENTRY logDepthRange (GLclampd zNear, GLclampd zFar){

	fprintf(glwState.logFile, "glDepthRange( %g, %g )\n", zNear, zFar);
	dllDepthRange(zNear, zFar);
}

static GLvoid APIENTRY logDisable (GLenum cap){

	char	*c;

	switch (cap){
	case GL_ALPHA_TEST:					c = "GL_ALPHA_TEST";			break;
	case GL_BLEND:						c = "GL_BLEND";					break;
	case GL_COLOR_MATERIAL:				c = "GL_COLOR_MATERIAL";		break;
	case GL_CULL_FACE:					c = "GL_CULL_FACE";				break;
	case GL_DEPTH_BOUNDS_TEST_EXT:		c = "GL_DEPTH_BOUNDS_TEST";		break;
	case GL_DEPTH_TEST:					c = "GL_DEPTH_TEST";			break;
	case GL_DITHER:						c = "GL_DITHER";				break;
	case GL_FOG:						c = "GL_FOG";					break;
	case GL_FRAGMENT_PROGRAM_ARB:		c = "GL_FRAGMENT_PROGRAM";		break;
	case GL_FRAGMENT_SHADER_ATI:		c = "GL_FRAGMENT_SHADER";		break;
	case GL_LIGHTING:					c = "GL_LIGHTING";				break;
	case GL_MULTISAMPLE_ARB:			c = "GL_MULTISAMPLE";			break;
	case GL_NORMALIZE:					c = "GL_NORMALIZE";				break;
	case GL_POLYGON_OFFSET_FILL:		c = "GL_POLYGON_OFFSET_FILL";	break;
	case GL_POLYGON_OFFSET_LINE:		c = "GL_POLYGON_OFFSET_LINE";	break;
	case GL_POLYGON_OFFSET_POINT:		c = "GL_POLYGON_OFFSET_POINT";	break;
	case GL_REGISTER_COMBINERS_NV:		c = "GL_REGISTER_COMBINERS";	break;
	case GL_STENCIL_TEST:				c = "GL_STENCIL_TEST";			break;
	case GL_STENCIL_TEST_TWO_SIDE_EXT:	c = "GL_STENCIL_TEST_TWO_SIDE";	break;
	case GL_SCISSOR_TEST:				c = "GL_SCISSOR_TEST";			break;
	case GL_TEXTURE_1D:					c = "GL_TEXTURE_1D";			break;
	case GL_TEXTURE_2D:					c = "GL_TEXTURE_2D";			break;
	case GL_TEXTURE_3D:					c = "GL_TEXTURE_3D";			break;
	case GL_TEXTURE_CUBE_MAP_ARB:		c = "GL_TEXTURE_CUBE_MAP";		break;
	case GL_TEXTURE_GEN_Q:				c = "GL_TEXTURE_GEN_Q";			break;
	case GL_TEXTURE_GEN_R:				c = "GL_TEXTURE_GEN_R";			break;
	case GL_TEXTURE_GEN_S:				c = "GL_TEXTURE_GEN_S";			break;
	case GL_TEXTURE_GEN_T:				c = "GL_TEXTURE_GEN_T";			break;
	case GL_VERTEX_PROGRAM_ARB:			c = "GL_VERTEX_PROGRAM";		break;
	default:							c = va("0x%X", cap);			break;
	}

	if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
		c = va("GL_CLIP_PLANE%i", GL_CLIP_PLANE5 - cap);
	if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7)
		c = va("GL_LIGHT%i", GL_LIGHT7 - cap);

	fprintf(glwState.logFile, "glDisable( %s )\n", c);
	dllDisable(cap);
}

static GLvoid APIENTRY logDisableClientState (GLenum array){

	char	*a;

	switch (array){
	case GL_COLOR_ARRAY:			a = "GL_COLOR_ARRAY";			break;
	case GL_TEXTURE_COORD_ARRAY:	a = "GL_TEXTURE_COORD_ARRAY";	break;
	case GL_NORMAL_ARRAY:			a = "GL_NORMAL_ARRAY";			break;
	case GL_VERTEX_ARRAY:			a = "GL_VERTEX_ARRAY";			break;
	default:						a = va("0x%X", array);			break;
	}

	fprintf(glwState.logFile, "glDisableClientState( %s )\n", a);
	dllDisableClientState(array);
}

static GLvoid APIENTRY logDrawArrays (GLenum mode, GLint first, GLsizei count){

	fprintf(glwState.logFile, "glDrawArrays\n");
	dllDrawArrays(mode, first, count);
}

static GLvoid APIENTRY logDrawBuffer (GLenum mode){

	char	*m;

	switch (mode){
	case GL_NONE:			m = "GL_NONE";				break;
	case GL_FRONT_LEFT:		m = "GL_FRONT_LEFT";		break;
	case GL_FRONT_RIGHT:	m = "GL_FRONT_RIGHT";		break;
	case GL_BACK_LEFT:		m = "GL_BACK_LEFT";			break;
	case GL_BACK_RIGHT:		m = "GL_BACK_RIGHT";		break;
	case GL_FRONT:			m = "GL_FRONT";				break;
	case GL_BACK:			m = "GL_BACK";				break;
	case GL_LEFT:			m = "GL_LEFT";				break;
	case GL_RIGHT:			m = "GL_RIGHT";				break;
	case GL_FRONT_AND_BACK:	m = "GL_FRONT_AND_BACK";	break;
	default:				m = va("0x%X", mode);		break;
	}

	fprintf(glwState.logFile, "glDrawBuffer( %s )\n", m);
	dllDrawBuffer(mode);
}

static GLvoid APIENTRY logDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices){

	char	*m, *t;

	switch (mode){
	case GL_POINTS:			m = "GL_POINTS";			break;
	case GL_LINES:			m = "GL_LINES";				break;
	case GL_LINE_STRIP:		m = "GL_LINE_STRIP";		break;
	case GL_LINE_LOOP:		m = "GL_LINE_LOOP";			break;
	case GL_TRIANGLES:		m = "GL_TRIANGLES";			break;
	case GL_TRIANGLE_STRIP:	m = "GL_TRIANGLE_STRIP";	break;
	case GL_TRIANGLE_FAN:	m = "GL_TRIANGLE_FAN";		break;
	case GL_QUADS:			m = "GL_QUADS";				break;
	case GL_QUAD_STRIP:		m = "GL_QUAD_STRIP";		break;
	case GL_POLYGON:		m = "GL_POLYGON";			break;
	default:				m = va("0x%X", mode);		break;
	}

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glDrawElements( %s, %i, %s, %p )\n", m, count, t, indices);
	dllDrawElements(mode, count, type, indices);
}

static GLvoid APIENTRY logDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){

	fprintf(glwState.logFile, "glDrawPixels\n");
	dllDrawPixels(width, height, format, type, pixels);
}

static GLvoid APIENTRY logEdgeFlag (GLboolean flag){

	fprintf(glwState.logFile, "glEdgeFlag\n");
	dllEdgeFlag(flag);
}

static GLvoid APIENTRY logEdgeFlagPointer (GLsizei stride, const GLvoid *pointer){

	fprintf(glwState.logFile, "glEdgeFlagPointer\n");
	dllEdgeFlagPointer(stride, pointer);
}

static GLvoid APIENTRY logEdgeFlagv (const GLboolean *flag){

	fprintf(glwState.logFile, "glEdgeFlagv\n");
	dllEdgeFlagv(flag);
}

static GLvoid APIENTRY logEnable (GLenum cap){

	char	*c;

	switch (cap){
	case GL_ALPHA_TEST:					c = "GL_ALPHA_TEST";			break;
	case GL_BLEND:						c = "GL_BLEND";					break;
	case GL_COLOR_MATERIAL:				c = "GL_COLOR_MATERIAL";		break;
	case GL_CULL_FACE:					c = "GL_CULL_FACE";				break;
	case GL_DEPTH_BOUNDS_TEST_EXT:		c = "GL_DEPTH_BOUNDS_TEST";		break;
	case GL_DEPTH_TEST:					c = "GL_DEPTH_TEST";			break;
	case GL_DITHER:						c = "GL_DITHER";				break;
	case GL_FOG:						c = "GL_FOG";					break;
	case GL_FRAGMENT_PROGRAM_ARB:		c = "GL_FRAGMENT_PROGRAM";		break;
	case GL_FRAGMENT_SHADER_ATI:		c = "GL_FRAGMENT_SHADER";		break;
	case GL_LIGHTING:					c = "GL_LIGHTING";				break;
	case GL_MULTISAMPLE_ARB:			c = "GL_MULTISAMPLE";			break;
	case GL_NORMALIZE:					c = "GL_NORMALIZE";				break;
	case GL_POLYGON_OFFSET_FILL:		c = "GL_POLYGON_OFFSET_FILL";	break;
	case GL_POLYGON_OFFSET_LINE:		c = "GL_POLYGON_OFFSET_LINE";	break;
	case GL_POLYGON_OFFSET_POINT:		c = "GL_POLYGON_OFFSET_POINT";	break;
	case GL_REGISTER_COMBINERS_NV:		c = "GL_REGISTER_COMBINERS";	break;
	case GL_STENCIL_TEST:				c = "GL_STENCIL_TEST";			break;
	case GL_STENCIL_TEST_TWO_SIDE_EXT:	c = "GL_STENCIL_TEST_TWO_SIDE";	break;
	case GL_SCISSOR_TEST:				c = "GL_SCISSOR_TEST";			break;
	case GL_TEXTURE_1D:					c = "GL_TEXTURE_1D";			break;
	case GL_TEXTURE_2D:					c = "GL_TEXTURE_2D";			break;
	case GL_TEXTURE_3D:					c = "GL_TEXTURE_3D";			break;
	case GL_TEXTURE_CUBE_MAP_ARB:		c = "GL_TEXTURE_CUBE_MAP";		break;
	case GL_TEXTURE_GEN_Q:				c = "GL_TEXTURE_GEN_Q";			break;
	case GL_TEXTURE_GEN_R:				c = "GL_TEXTURE_GEN_R";			break;
	case GL_TEXTURE_GEN_S:				c = "GL_TEXTURE_GEN_S";			break;
	case GL_TEXTURE_GEN_T:				c = "GL_TEXTURE_GEN_T";			break;
	case GL_VERTEX_PROGRAM_ARB:			c = "GL_VERTEX_PROGRAM";		break;
	default:							c = va("0x%X", cap);			break;
	}

	if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
		c = va("GL_CLIP_PLANE%i", GL_CLIP_PLANE5 - cap);
	if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7)
		c = va("GL_LIGHT%i", GL_LIGHT7 - cap);

	fprintf(glwState.logFile, "glEnable( %s )\n", c);
	dllEnable(cap);
}

static GLvoid APIENTRY logEnableClientState (GLenum array){

	char	*a;

	switch (array){
	case GL_COLOR_ARRAY:			a = "GL_COLOR_ARRAY";			break;
	case GL_TEXTURE_COORD_ARRAY:	a = "GL_TEXTURE_COORD_ARRAY";	break;
	case GL_NORMAL_ARRAY:			a = "GL_NORMAL_ARRAY";			break;
	case GL_VERTEX_ARRAY:			a = "GL_VERTEX_ARRAY";			break;
	default:						a = va("0x%X", array);			break;
	}

	fprintf(glwState.logFile, "glEnableClientState( %s )\n", a);
	dllEnableClientState(array);
}

static GLvoid APIENTRY logEnd (GLvoid){

	fprintf(glwState.logFile, "glEnd\n");
	dllEnd();
}

static GLvoid APIENTRY logEndList (GLvoid){

	fprintf(glwState.logFile, "glEndList\n");
	dllEndList();
}

static GLvoid APIENTRY logEvalCoord1d (GLdouble u){

	fprintf(glwState.logFile, "glEvalCoord1d\n");
	dllEvalCoord1d(u);
}

static GLvoid APIENTRY logEvalCoord1dv (const GLdouble *u){

	fprintf(glwState.logFile, "glEvalCoord1dv\n");
	dllEvalCoord1dv(u);
}

static GLvoid APIENTRY logEvalCoord1f (GLfloat u){

	fprintf(glwState.logFile, "glEvalCoord1f\n");
	dllEvalCoord1f(u);
}

static GLvoid APIENTRY logEvalCoord1fv (const GLfloat *u){

	fprintf(glwState.logFile, "glEvalCoord1fv\n");
	dllEvalCoord1fv(u);
}

static GLvoid APIENTRY logEvalCoord2d (GLdouble u, GLdouble v){

	fprintf(glwState.logFile, "glEvalCoord2d\n");
	dllEvalCoord2d(u, v);
}

static GLvoid APIENTRY logEvalCoord2dv (const GLdouble *u){

	fprintf(glwState.logFile, "glEvalCoord2dv\n");
	dllEvalCoord2dv(u);
}

static GLvoid APIENTRY logEvalCoord2f (GLfloat u, GLfloat v){

	fprintf(glwState.logFile, "glEvalCoord2f\n");
	dllEvalCoord2f(u, v);
}

static GLvoid APIENTRY logEvalCoord2fv (const GLfloat *u){

	fprintf(glwState.logFile, "glEvalCoord2fv\n");
	dllEvalCoord2fv(u);
}

static GLvoid APIENTRY logEvalMesh1 (GLenum mode, GLint i1, GLint i2){

	fprintf(glwState.logFile, "glEvalMesh1\n");
	dllEvalMesh1(mode, i1, i2);
}

static GLvoid APIENTRY logEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2){

	fprintf(glwState.logFile, "glEvalMesh2\n");
	dllEvalMesh2(mode, i1, i2, j1, j2);
}

static GLvoid APIENTRY logEvalPoint1 (GLint i){

	fprintf(glwState.logFile, "glEvalPoint1\n");
	dllEvalPoint1(i);
}

static GLvoid APIENTRY logEvalPoint2 (GLint i, GLint j){

	fprintf(glwState.logFile, "glEvalPoint2\n");
	dllEvalPoint2(i, j);
}

static GLvoid APIENTRY logFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer){

	fprintf(glwState.logFile, "glFeedbackBuffer\n");
	dllFeedbackBuffer(size, type, buffer);
}

static GLvoid APIENTRY logFinish (GLvoid){

	fprintf(glwState.logFile, "glFinish\n");
	dllFinish();
}

static GLvoid APIENTRY logFlush (GLvoid){

	fprintf(glwState.logFile, "glFlush\n");
	dllFlush();
}

static GLvoid APIENTRY logFogf (GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glFogf\n");
	dllFogf(pname, param);
}

static GLvoid APIENTRY logFogfv (GLenum pname, const GLfloat *params){

	fprintf(glwState.logFile, "glFogfv\n");
	dllFogfv(pname, params);
}

static GLvoid APIENTRY logFogi (GLenum pname, GLint param){

	fprintf(glwState.logFile, "glFogi\n");
	dllFogi(pname, param);
}

static GLvoid APIENTRY logFogiv (GLenum pname, const GLint *params){

	fprintf(glwState.logFile, "glFogiv\n");
	dllFogiv(pname, params);
}

static GLvoid APIENTRY logFrontFace (GLenum mode){

	char	*m;

	switch (mode){
	case GL_CW:		m = "GL_CW";			break;
	case GL_CCW:	m = "GL_CCW";			break;
	default:		m = va("0x%X", mode);	break;
	}

	fprintf(glwState.logFile, "glFrontFace( %s )\n", m);
	dllFrontFace(mode);
}

static GLvoid APIENTRY logFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){

	fprintf(glwState.logFile, "glFrustum( %g, %g, %g, %g, %g, %g )\n", left, right, bottom, top, zNear, zFar);
	dllFrustum(left, right, bottom, top, zNear, zFar);
}

static GLuint APIENTRY logGenLists (GLsizei range){

	fprintf(glwState.logFile, "glGenLists\n");
	return dllGenLists(range);
}

static GLvoid APIENTRY logGenTextures (GLsizei n, GLuint *textures){

	fprintf(glwState.logFile, "glGenTextures\n");
	dllGenTextures(n, textures);
}

static GLvoid APIENTRY logGetBooleanv (GLenum pname, GLboolean *params){

	fprintf(glwState.logFile, "glGetBooleanv\n");
	dllGetBooleanv(pname, params);
}

static GLvoid APIENTRY logGetClipPlane (GLenum plane, GLdouble *equation){

	fprintf(glwState.logFile, "glGetClipPlane\n");
	dllGetClipPlane(plane, equation);
}

static GLvoid APIENTRY logGetDoublev (GLenum pname, GLdouble *params){

	fprintf(glwState.logFile, "glGetDoublev\n");
	dllGetDoublev(pname, params);
}

static GLenum APIENTRY logGetError (GLvoid){

	fprintf(glwState.logFile, "glGetError\n");
	return dllGetError();
}

static GLvoid APIENTRY logGetFloatv (GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetFloatv\n");
	dllGetFloatv(pname, params);
}

static GLvoid APIENTRY logGetIntegerv (GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetIntegerv\n");
	dllGetIntegerv(pname, params);
}

static GLvoid APIENTRY logGetLightfv (GLenum light, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetLightfv\n");
	dllGetLightfv(light, pname, params);
}

static GLvoid APIENTRY logGetLightiv (GLenum light, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetLightiv\n");
	dllGetLightiv(light, pname, params);
}

static GLvoid APIENTRY logGetMapdv (GLenum target, GLenum query, GLdouble *v){

	fprintf(glwState.logFile, "glGetMapdv\n");
	dllGetMapdv(target, query, v);
}

static GLvoid APIENTRY logGetMapfv (GLenum target, GLenum query, GLfloat *v){

	fprintf(glwState.logFile, "glGetMapfv\n");
	dllGetMapfv(target, query, v);
}

static GLvoid APIENTRY logGetMapiv (GLenum target, GLenum query, GLint *v){

	fprintf(glwState.logFile, "glGetMapiv\n");
	dllGetMapiv(target, query, v);
}

static GLvoid APIENTRY logGetMaterialfv (GLenum face, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetMaterialfv\n");
	dllGetMaterialfv(face, pname, params);
}

static GLvoid APIENTRY logGetMaterialiv (GLenum face, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetMaterialiv\n");
	dllGetMaterialiv(face, pname, params);
}

static GLvoid APIENTRY logGetPixelMapfv (GLenum map, GLfloat *values){

	fprintf(glwState.logFile, "glGetPixelMapfv\n");
	dllGetPixelMapfv(map, values);
}

static GLvoid APIENTRY logGetPixelMapuiv (GLenum map, GLuint *values){

	fprintf(glwState.logFile, "glGetPixelMapuiv\n");
	dllGetPixelMapuiv(map, values);
}

static GLvoid APIENTRY logGetPixelMapusv (GLenum map, GLushort *values){

	fprintf(glwState.logFile, "glGetPixelMapusv\n");
	dllGetPixelMapusv(map, values);
}

static GLvoid APIENTRY logGetPointerv (GLenum pname, GLvoid* *params){

	fprintf(glwState.logFile, "glGetPointerv\n");
	dllGetPointerv(pname, params);
}

static GLvoid APIENTRY logGetPolygonStipple (GLubyte *mask){

	fprintf(glwState.logFile, "glGetPolygonStipple\n");
	dllGetPolygonStipple(mask);
}

static const GLubyte * APIENTRY logGetString (GLenum name){

	fprintf(glwState.logFile, "glGetString\n");
	return dllGetString(name);
}

static GLvoid APIENTRY logGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetTexEnvfv\n");
	dllGetTexEnvfv(target, pname, params);
}

static GLvoid APIENTRY logGetTexEnviv (GLenum target, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetTexEnviv\n");
	dllGetTexEnviv(target, pname, params);
}

static GLvoid APIENTRY logGetTexGendv (GLenum coord, GLenum pname, GLdouble *params){

	fprintf(glwState.logFile, "glGetTexGendv\n");
	dllGetTexGendv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetTexGenfv\n");
	dllGetTexGenfv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexGeniv (GLenum coord, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetTexGeniv\n");
	dllGetTexGeniv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels){

	fprintf(glwState.logFile, "glGetTexImage\n");
	dllGetTexImage(target, level, format, type, pixels);
}

static GLvoid APIENTRY logGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetTexLevelParameterfv\n");
	dllGetTexLevelParameterfv(target, level, pname, params);
}

static GLvoid APIENTRY logGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetTexLevelParameteriv\n");
	dllGetTexLevelParameteriv(target, level, pname, params);
}

static GLvoid APIENTRY logGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params){

	fprintf(glwState.logFile, "glGetTexParameterfv\n");
	dllGetTexParameterfv(target, pname, params);
}

static GLvoid APIENTRY logGetTexParameteriv (GLenum target, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetTexParameteriv\n");
	dllGetTexParameteriv(target, pname, params);
}

static GLvoid APIENTRY logHint (GLenum target, GLenum mode){

	fprintf(glwState.logFile, "glHint\n");
	dllHint(target, mode);
}

static GLvoid APIENTRY logIndexMask (GLuint mask){

	fprintf(glwState.logFile, "glIndexMask\n");
	dllIndexMask(mask);
}

static GLvoid APIENTRY logIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer){

	fprintf(glwState.logFile, "glIndexPointer\n");
	dllIndexPointer(type, stride, pointer);
}

static GLvoid APIENTRY logIndexd (GLdouble c){

	fprintf(glwState.logFile, "glIndexd\n");
	dllIndexd(c);
}

static GLvoid APIENTRY logIndexdv (const GLdouble *c){

	fprintf(glwState.logFile, "glIndexdv\n");
	dllIndexdv(c);
}

static GLvoid APIENTRY logIndexf (GLfloat c){

	fprintf(glwState.logFile, "glIndexf\n");
	dllIndexf(c);
}

static GLvoid APIENTRY logIndexfv (const GLfloat *c){

	fprintf(glwState.logFile, "glIndexfv\n");
	dllIndexfv(c);
}

static GLvoid APIENTRY logIndexi (GLint c){

	fprintf(glwState.logFile, "glIndexi\n");
	dllIndexi(c);
}

static GLvoid APIENTRY logIndexiv (const GLint *c){

	fprintf(glwState.logFile, "glIndexiv\n");
	dllIndexiv(c);
}

static GLvoid APIENTRY logIndexs (GLshort c){

	fprintf(glwState.logFile, "glIndexs\n");
	dllIndexs(c);
}

static GLvoid APIENTRY logIndexsv (const GLshort *c){

	fprintf(glwState.logFile, "glIndexsv\n");
	dllIndexsv(c);
}

static GLvoid APIENTRY logIndexub (GLubyte c){

	fprintf(glwState.logFile, "glIndexub\n");
	dllIndexub(c);
}

static GLvoid APIENTRY logIndexubv (const GLubyte *c){

	fprintf(glwState.logFile, "glIndexubv\n");
	dllIndexubv(c);
}

static GLvoid APIENTRY logInitNames (GLvoid){

	fprintf(glwState.logFile, "glInitNames\n");
	dllInitNames();
}

static GLvoid APIENTRY logInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer){

	fprintf(glwState.logFile, "glInterleavedArrays\n");
	dllInterleavedArrays(format, stride, pointer);
}

static GLboolean APIENTRY logIsEnabled (GLenum cap){

	fprintf(glwState.logFile, "glIsEnabled\n");
	return dllIsEnabled(cap);
}

static GLboolean APIENTRY logIsList (GLuint list){

	fprintf(glwState.logFile, "glIsList\n");
	return dllIsList(list);
}

static GLboolean APIENTRY logIsTexture (GLuint texture){

	fprintf(glwState.logFile, "glIsTexture\n");
	return dllIsTexture(texture);
}

static GLvoid APIENTRY logLightModelf (GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glLightModelf\n");
	dllLightModelf(pname, param);
}

static GLvoid APIENTRY logLightModelfv (GLenum pname, const GLfloat *params){

	fprintf(glwState.logFile, "glLightModelfv\n");
	dllLightModelfv(pname, params);
}

static GLvoid APIENTRY logLightModeli (GLenum pname, GLint param){

	fprintf(glwState.logFile, "glLightModeli\n");
	dllLightModeli(pname, param);
}

static GLvoid APIENTRY logLightModeliv (GLenum pname, const GLint *params){

	fprintf(glwState.logFile, "glLightModeliv\n");
	dllLightModeliv(pname, params);
}

static GLvoid APIENTRY logLightf (GLenum light, GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glLightf\n");
	dllLightf(light, pname, param);
}

static GLvoid APIENTRY logLightfv (GLenum light, GLenum pname, const GLfloat *params){

	fprintf(glwState.logFile, "glLightfv\n");
	dllLightfv(light, pname, params);
}

static GLvoid APIENTRY logLighti (GLenum light, GLenum pname, GLint param){

	fprintf(glwState.logFile, "glLighti\n");
	dllLighti(light, pname, param);
}

static GLvoid APIENTRY logLightiv (GLenum light, GLenum pname, const GLint *params){

	fprintf(glwState.logFile, "glLightiv\n");
	dllLightiv(light, pname, params);
}

static GLvoid APIENTRY logLineStipple (GLint factor, GLushort pattern){

	fprintf(glwState.logFile, "glLineStipple\n");
	dllLineStipple(factor, pattern);
}

static GLvoid APIENTRY logLineWidth (GLfloat width){

	fprintf(glwState.logFile, "glLineWidth\n");
	dllLineWidth(width);
}

static GLvoid APIENTRY logListBase (GLuint base){

	fprintf(glwState.logFile, "glListBase\n");
	dllListBase(base);
}

static GLvoid APIENTRY logLoadIdentity (GLvoid){

	fprintf(glwState.logFile, "glLoadIdentity\n");
	dllLoadIdentity();
}

static GLvoid APIENTRY logLoadMatrixd (const GLdouble *m){

	fprintf(glwState.logFile, "glLoadMatrixd\n");
	dllLoadMatrixd(m);
}

static GLvoid APIENTRY logLoadMatrixf (const GLfloat *m){

	fprintf(glwState.logFile, "glLoadMatrixf\n");
	dllLoadMatrixf(m);
}

static GLvoid APIENTRY logLoadName (GLuint name){

	fprintf(glwState.logFile, "glLoadName\n");
	dllLoadName(name);
}

static GLvoid APIENTRY logLogicOp (GLenum opcode){

	fprintf(glwState.logFile, "glLogicOp\n");
	dllLogicOp(opcode);
}

static GLvoid APIENTRY logMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points){

	fprintf(glwState.logFile, "glMap1d\n");
	dllMap1d(target, u1, u2, stride, order, points);
}

static GLvoid APIENTRY logMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points){

	fprintf(glwState.logFile, "glMap1f\n");
	dllMap1f(target, u1, u2, stride, order, points);
}

static GLvoid APIENTRY logMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points){

	fprintf(glwState.logFile, "glMap2d\n");
	dllMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static GLvoid APIENTRY logMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points){

	fprintf(glwState.logFile, "glMap2f\n");
	dllMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static GLvoid APIENTRY logMapGrid1d (GLint un, GLdouble u1, GLdouble u2){

	fprintf(glwState.logFile, "glMapGrid1d\n");
	dllMapGrid1d(un, u1, u2);
}

static GLvoid APIENTRY logMapGrid1f (GLint un, GLfloat u1, GLfloat u2){

	fprintf(glwState.logFile, "glMapGrid1f\n");
	dllMapGrid1f(un, u1, u2);
}

static GLvoid APIENTRY logMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2){

	fprintf(glwState.logFile, "glMapGrid2d\n");
	dllMapGrid2d(un, u1, u2, vn, v1, v2);
}

static GLvoid APIENTRY logMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2){

	fprintf(glwState.logFile, "glMapGrid2f\n");
	dllMapGrid2f(un, u1, u2, vn, v1, v2);
}

static GLvoid APIENTRY logMaterialf (GLenum face, GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glMaterialf\n");
	dllMaterialf(face, pname, param);
}

static GLvoid APIENTRY logMaterialfv (GLenum face, GLenum pname, const GLfloat *params){

	fprintf(glwState.logFile, "glMaterialfv\n");
	dllMaterialfv(face, pname, params);
}

static GLvoid APIENTRY logMateriali (GLenum face, GLenum pname, GLint param){

	fprintf(glwState.logFile, "glMateriali\n");
	dllMateriali(face, pname, param);
}

static GLvoid APIENTRY logMaterialiv (GLenum face, GLenum pname, const GLint *params){

	fprintf(glwState.logFile, "glMaterialiv\n");
	dllMaterialiv(face, pname, params);
}

static GLvoid APIENTRY logMatrixMode (GLenum mode){

	char	*m;

	switch (mode){
	case GL_PROJECTION:	m = "GL_PROJECTION";	break;
	case GL_MODELVIEW:	m = "GL_MODELVIEW";		break;
	case GL_TEXTURE:	m = "GL_TEXTURE";		break;
	default:			m = va("0x%X", mode);	break;
	}

	if (mode >= GL_MATRIX0_ARB && mode <= GL_MATRIX31_ARB)
		m = va("GL_MATRIX%i", mode - GL_MATRIX0_ARB);

	fprintf(glwState.logFile, "glMatrixMode( %s )\n", m);
	dllMatrixMode(mode);
}

static GLvoid APIENTRY logMultMatrixd (const GLdouble *m){

	fprintf(glwState.logFile, "glMultMatrixd\n");
	dllMultMatrixd(m);
}

static GLvoid APIENTRY logMultMatrixf (const GLfloat *m){

	fprintf(glwState.logFile, "glMultMatrixf\n");
	dllMultMatrixf(m);
}

static GLvoid APIENTRY logNewList (GLuint list, GLenum mode){

	fprintf(glwState.logFile, "glNewList\n");
	dllNewList(list, mode);
}

static GLvoid APIENTRY logNormal3b (GLbyte nx, GLbyte ny, GLbyte nz){

	fprintf(glwState.logFile, "glNormal3b\n");
	dllNormal3b(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3bv (const GLbyte *v){

	fprintf(glwState.logFile, "glNormal3bv\n");
	dllNormal3bv(v);
}

static GLvoid APIENTRY logNormal3d (GLdouble nx, GLdouble ny, GLdouble nz){

	fprintf(glwState.logFile, "glNormal3d\n");
	dllNormal3d(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3dv (const GLdouble *v){

	fprintf(glwState.logFile, "glNormal3dv\n");
	dllNormal3dv(v);
}

static GLvoid APIENTRY logNormal3f (GLfloat nx, GLfloat ny, GLfloat nz){

	fprintf(glwState.logFile, "glNormal3f\n");
	dllNormal3f(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3fv (const GLfloat *v){

	fprintf(glwState.logFile, "glNormal3fv\n");
	dllNormal3fv(v);
}

static GLvoid APIENTRY logNormal3i (GLint nx, GLint ny, GLint nz){

	fprintf(glwState.logFile, "glNormal3i\n");
	dllNormal3i(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3iv (const GLint *v){

	fprintf(glwState.logFile, "glNormal3iv\n");
	dllNormal3iv(v);
}

static GLvoid APIENTRY logNormal3s (GLshort nx, GLshort ny, GLshort nz){

	fprintf(glwState.logFile, "glNormal3s\n");
	dllNormal3s(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3sv (const GLshort *v){

	fprintf(glwState.logFile, "glNormal3sv\n");
	dllNormal3sv(v);
}

static GLvoid APIENTRY logNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer){

	char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glNormalPointer( %s, %i, %p )\n", t, stride, pointer);
	dllNormalPointer(type, stride, pointer);
}

static GLvoid APIENTRY logOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){

	fprintf(glwState.logFile, "glOrtho( %g, %g, %g, %g, %g, %g )\n", left, right, bottom, top, zNear, zFar);
	dllOrtho(left, right, bottom, top, zNear, zFar);
}

static GLvoid APIENTRY logPassThrough (GLfloat token){

	fprintf(glwState.logFile, "glPassThrough\n");
	dllPassThrough(token);
}

static GLvoid APIENTRY logPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values){

	fprintf(glwState.logFile, "glPixelMapfv\n");
	dllPixelMapfv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values){

	fprintf(glwState.logFile, "glPixelMapuiv\n");
	dllPixelMapuiv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values){

	fprintf(glwState.logFile, "glPixelMapusv\n");
	dllPixelMapusv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelStoref (GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glPixelStoref\n");
	dllPixelStoref(pname, param);
}

static GLvoid APIENTRY logPixelStorei (GLenum pname, GLint param){

	fprintf(glwState.logFile, "glPixelStorei\n");
	dllPixelStorei(pname, param);
}

static GLvoid APIENTRY logPixelTransferf (GLenum pname, GLfloat param){

	fprintf(glwState.logFile, "glPixelTransferf\n");
	dllPixelTransferf(pname, param);
}

static GLvoid APIENTRY logPixelTransferi (GLenum pname, GLint param){

	fprintf(glwState.logFile, "glPixelTransferi\n");
	dllPixelTransferi(pname, param);
}

static GLvoid APIENTRY logPixelZoom (GLfloat xfactor, GLfloat yfactor){

	fprintf(glwState.logFile, "glPixelZoom\n");
	dllPixelZoom(xfactor, yfactor);
}

static GLvoid APIENTRY logPointSize (GLfloat size){

	fprintf(glwState.logFile, "glPointSize\n");
	dllPointSize(size);
}

static GLvoid APIENTRY logPolygonMode (GLenum face, GLenum mode){

	char	*f, *m;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";				break;
	case GL_BACK:			f = "GL_BACK";				break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";	break;
	default:				f = va("0x%X", face);		break;
	}

	switch (mode){
	case GL_POINT:			m = "GL_POINT";				break;
	case GL_LINE:			m = "GL_LINE";				break;
	case GL_FILL:			m = "GL_FILL";				break;
	default:				m = va("0x%X", mode);		break;
	}

	fprintf(glwState.logFile, "glPolygonMode( %s, %s )\n", f, m);
	dllPolygonMode(face, mode);
}

static GLvoid APIENTRY logPolygonOffset (GLfloat factor, GLfloat units){

	fprintf(glwState.logFile, "glPolygonOffset( %g, %g )\n", factor, units);
	dllPolygonOffset(factor, units);
}

static GLvoid APIENTRY logPolygonStipple (const GLubyte *mask){

	fprintf(glwState.logFile, "glPolygonStipple\n");
	dllPolygonStipple(mask);
}

static GLvoid APIENTRY logPopAttrib (GLvoid){

	fprintf(glwState.logFile, "glPopAttrib\n");
	dllPopAttrib();
}

static GLvoid APIENTRY logPopClientAttrib (GLvoid){

	fprintf(glwState.logFile, "glPopClientAttrib\n");
	dllPopClientAttrib();
}

static GLvoid APIENTRY logPopMatrix (GLvoid){

	fprintf(glwState.logFile, "glPopMatrix\n");
	dllPopMatrix();
}

static GLvoid APIENTRY logPopName (GLvoid){

	fprintf(glwState.logFile, "glPopName\n");
	dllPopName();
}

static GLvoid APIENTRY logPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities){

	fprintf(glwState.logFile, "glPrioritizeTextures\n");
	dllPrioritizeTextures(n, textures, priorities);
}

static GLvoid APIENTRY logPushAttrib (GLbitfield mask){

	fprintf(glwState.logFile, "glPushAttrib\n");
	dllPushAttrib(mask);
}

static GLvoid APIENTRY logPushClientAttrib (GLbitfield mask){

	fprintf(glwState.logFile, "glPushClientAttrib\n");
	dllPushClientAttrib(mask);
}

static GLvoid APIENTRY logPushMatrix (GLvoid){

	fprintf(glwState.logFile, "glPushMatrix\n");
	dllPushMatrix();
}

static GLvoid APIENTRY logPushName (GLuint name){

	fprintf(glwState.logFile, "glPushName\n");
	dllPushName(name);
}

static GLvoid APIENTRY logRasterPos2d (GLdouble x, GLdouble y){

	fprintf(glwState.logFile, "glRasterPot2d\n");
	dllRasterPos2d(x, y);
}

static GLvoid APIENTRY logRasterPos2dv (const GLdouble *v){

	fprintf(glwState.logFile, "glRasterPos2dv\n");
	dllRasterPos2dv(v);
}

static GLvoid APIENTRY logRasterPos2f (GLfloat x, GLfloat y){

	fprintf(glwState.logFile, "glRasterPos2f\n");
	dllRasterPos2f(x, y);
}

static GLvoid APIENTRY logRasterPos2fv (const GLfloat *v){

	fprintf(glwState.logFile, "glRasterPos2dv\n");
	dllRasterPos2fv(v);
}

static GLvoid APIENTRY logRasterPos2i (GLint x, GLint y){

	fprintf(glwState.logFile, "glRasterPos2if\n");
	dllRasterPos2i(x, y);
}

static GLvoid APIENTRY logRasterPos2iv (const GLint *v){

	fprintf(glwState.logFile, "glRasterPos2iv\n");
	dllRasterPos2iv(v);
}

static GLvoid APIENTRY logRasterPos2s (GLshort x, GLshort y){

	fprintf(glwState.logFile, "glRasterPos2s\n");
	dllRasterPos2s(x, y);
}

static GLvoid APIENTRY logRasterPos2sv (const GLshort *v){

	fprintf(glwState.logFile, "glRasterPos2sv\n");
	dllRasterPos2sv(v);
}

static GLvoid APIENTRY logRasterPos3d (GLdouble x, GLdouble y, GLdouble z){

	fprintf(glwState.logFile, "glRasterPos3d\n");
	dllRasterPos3d(x, y, z);
}

static GLvoid APIENTRY logRasterPos3dv (const GLdouble *v){

	fprintf(glwState.logFile, "glRasterPos3dv\n");
	dllRasterPos3dv(v);
}

static GLvoid APIENTRY logRasterPos3f (GLfloat x, GLfloat y, GLfloat z){

	fprintf(glwState.logFile, "glRasterPos3f\n");
	dllRasterPos3f(x, y, z);
}

static GLvoid APIENTRY logRasterPos3fv (const GLfloat *v){

	fprintf(glwState.logFile, "glRasterPos3fv\n");
	dllRasterPos3fv(v);
}

static GLvoid APIENTRY logRasterPos3i (GLint x, GLint y, GLint z){

	fprintf(glwState.logFile, "glRasterPos3i\n");
	dllRasterPos3i(x, y, z);
}

static GLvoid APIENTRY logRasterPos3iv (const GLint *v){

	fprintf(glwState.logFile, "glRasterPos3iv\n");
	dllRasterPos3iv(v);
}

static GLvoid APIENTRY logRasterPos3s (GLshort x, GLshort y, GLshort z){

	fprintf(glwState.logFile, "glRasterPos3s\n");
	dllRasterPos3s(x, y, z);
}

static GLvoid APIENTRY logRasterPos3sv (const GLshort *v){

	fprintf(glwState.logFile, "glRasterPos3sv\n");
	dllRasterPos3sv(v);
}

static GLvoid APIENTRY logRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w){

	fprintf(glwState.logFile, "glRasterPos4d\n");
	dllRasterPos4d(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4dv (const GLdouble *v){

	fprintf(glwState.logFile, "glRasterPos4dv\n");
	dllRasterPos4dv(v);
}

static GLvoid APIENTRY logRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	fprintf(glwState.logFile, "glRasterPos4f\n");
	dllRasterPos4f(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4fv (const GLfloat *v){

	fprintf(glwState.logFile, "glRasterPos4fv\n");
	dllRasterPos4fv(v);
}

static GLvoid APIENTRY logRasterPos4i (GLint x, GLint y, GLint z, GLint w){

	fprintf(glwState.logFile, "glRasterPos4i\n");
	dllRasterPos4i(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4iv (const GLint *v){

	fprintf(glwState.logFile, "glRasterPos4iv\n");
	dllRasterPos4iv(v);
}

static GLvoid APIENTRY logRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w){

	fprintf(glwState.logFile, "glRasterPos4s\n");
	dllRasterPos4s(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4sv (const GLshort *v){

	fprintf(glwState.logFile, "glRasterPos4sv\n");
	dllRasterPos4sv(v);
}

static GLvoid APIENTRY logReadBuffer (GLenum mode){

	char	*m;

	switch (mode){
	case GL_FRONT_LEFT:		m = "GL_FRONT_LEFT";		break;
	case GL_FRONT_RIGHT:	m = "GL_FRONT_RIGHT";		break;
	case GL_BACK_LEFT:		m = "GL_BACK_LEFT";			break;
	case GL_BACK_RIGHT:		m = "GL_BACK_RIGHT";		break;
	case GL_FRONT:			m = "GL_FRONT";				break;
	case GL_BACK:			m = "GL_BACK";				break;
	case GL_LEFT:			m = "GL_LEFT";				break;
	case GL_RIGHT:			m = "GL_RIGHT";				break;
	default:				m = va("0x%X", mode);		break;
	}

	fprintf(glwState.logFile, "glReadBuffer( %s )\n", m);
	dllReadBuffer(mode);
}

static GLvoid APIENTRY logReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels){

	fprintf(glwState.logFile, "glReadPixels\n");
	dllReadPixels(x, y, width, height, format, type, pixels);
}

static GLvoid APIENTRY logRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2){

	fprintf(glwState.logFile, "glRectd\n");
	dllRectd(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectdv (const GLdouble *v1, const GLdouble *v2){

	fprintf(glwState.logFile, "glRectdv\n");
	dllRectdv(v1, v2);
}

static GLvoid APIENTRY logRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){

	fprintf(glwState.logFile, "glRectf\n");
	dllRectf(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectfv (const GLfloat *v1, const GLfloat *v2){

	fprintf(glwState.logFile, "glRectfv\n");
	dllRectfv(v1, v2);
}

static GLvoid APIENTRY logRecti (GLint x1, GLint y1, GLint x2, GLint y2){

	fprintf(glwState.logFile, "glRecti\n");
	dllRecti(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectiv (const GLint *v1, const GLint *v2){

	fprintf(glwState.logFile, "glRectiv\n");
	dllRectiv(v1, v2);
}

static GLvoid APIENTRY logRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2){

	fprintf(glwState.logFile, "glRects\n");
	dllRects(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectsv (const GLshort *v1, const GLshort *v2){

	fprintf(glwState.logFile, "glRectsv\n");
	dllRectsv(v1, v2);
}

static GLint APIENTRY logRenderMode (GLenum mode){

	fprintf(glwState.logFile, "glRenderMode\n");
	return dllRenderMode(mode);
}

static GLvoid APIENTRY logRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z){

	fprintf(glwState.logFile, "glRotated\n");
	dllRotated(angle, x, y, z);
}

static GLvoid APIENTRY logRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z){

	fprintf(glwState.logFile, "glRotatef\n");
	dllRotatef(angle, x, y, z);
}

static GLvoid APIENTRY logScaled (GLdouble x, GLdouble y, GLdouble z){

	fprintf(glwState.logFile, "glScaled\n");
	dllScaled(x, y, z);
}

static GLvoid APIENTRY logScalef (GLfloat x, GLfloat y, GLfloat z){

	fprintf(glwState.logFile, "glScalef\n");
	dllScalef(x, y, z);
}

static GLvoid APIENTRY logScissor (GLint x, GLint y, GLsizei width, GLsizei height){

	fprintf(glwState.logFile, "glScissor( %i, %i, %i, %i )\n", x, y, width, height);
	dllScissor(x, y, width, height);
}

static GLvoid APIENTRY logSelectBuffer (GLsizei size, GLuint *buffer){

	fprintf(glwState.logFile, "glSelectBuffer\n");
	dllSelectBuffer(size, buffer);
}

static GLvoid APIENTRY logShadeModel (GLenum mode){

	char	*m;

	switch (mode){
	case GL_FLAT:	m = "GL_FLAT";			break;
	case GL_SMOOTH:	m = "GL_SMOOTH";		break;
	default:		m = va("0x%X", mode);	break;
	}

	fprintf(glwState.logFile, "glShadeModel( %s )\n", m);
	dllShadeModel(mode);
}

static GLvoid APIENTRY logStencilFunc (GLenum func, GLint ref, GLuint mask){

	char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";			break;
	case GL_LESS:		f = "GL_LESS";			break;
	case GL_LEQUAL:		f = "GL_LEQUAL";		break;
	case GL_GREATER:	f = "GL_GREATER";		break;
	case GL_GEQUAL:		f = "GL_GEQUAL";		break;
	case GL_EQUAL:		f = "GL_EQUAL";			break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";		break;
	case GL_ALWAYS:		f = "GL_ALWAYS";		break;
	default:			f = va("0x%X", func);	break;
	}

	fprintf(glwState.logFile, "glStencilFunc( %s, %i, %u )\n", f, ref, mask);
	dllStencilFunc(func, ref, mask);
}

static GLvoid APIENTRY logStencilMask (GLuint mask){

	fprintf(glwState.logFile, "glStencilMask( %u )\n", mask);
	dllStencilMask(mask);
}

static GLvoid APIENTRY logStencilOp (GLenum fail, GLenum zfail, GLenum zpass){

	char	*f, *zf, *zp;

	switch (fail){
	case GL_KEEP:			f = "GL_KEEP";			break;
	case GL_ZERO:			f = "GL_ZERO";			break;
	case GL_REPLACE:		f = "GL_REPLACE";		break;
	case GL_INCR:			f = "GL_INCR";			break;
	case GL_INCR_WRAP_EXT:	f = "GL_INCR_WRAP";		break;
	case GL_DECR:			f = "GL_DECR";			break;
	case GL_DECR_WRAP_EXT:	f = "GL_DECR_WRAP";		break;
	case GL_INVERT:			f = "GL_INVERT";		break;
	default:				f = va("0x%X", fail);	break;
	}

	switch (zfail){
	case GL_KEEP:			zf = "GL_KEEP";			break;
	case GL_ZERO:			zf = "GL_ZERO";			break;
	case GL_REPLACE:		zf = "GL_REPLACE";		break;
	case GL_INCR:			zf = "GL_INCR";			break;
	case GL_INCR_WRAP_EXT:	zf = "GL_INCR_WRAP";	break;
	case GL_DECR:			zf = "GL_DECR";			break;
	case GL_DECR_WRAP_EXT:	zf = "GL_DECR_WRAP";	break;
	case GL_INVERT:			zf = "GL_INVERT";		break;
	default:				zf = va("0x%X", zfail);	break;
	}

	switch (zpass){
	case GL_KEEP:			zp = "GL_KEEP";			break;
	case GL_ZERO:			zp = "GL_ZERO";			break;
	case GL_REPLACE:		zp = "GL_REPLACE";		break;
	case GL_INCR:			zp = "GL_INCR";			break;
	case GL_INCR_WRAP_EXT:	zp = "GL_INCR_WRAP";	break;
	case GL_DECR:			zp = "GL_DECR";			break;
	case GL_DECR_WRAP_EXT:	zp = "GL_DECR_WRAP";	break;
	case GL_INVERT:			zp = "GL_INVERT";		break;
	default:				zp = va("0x%X", zpass);	break;
	}

	fprintf(glwState.logFile, "glStencilOp( %s, %s, %s )\n", f, zf, zp);
	dllStencilOp(fail, zfail, zpass);
}

static GLvoid APIENTRY logTexCoord1d (GLdouble s){

	fprintf(glwState.logFile, "glTexCoord1d\n");
	dllTexCoord1d(s);
}

static GLvoid APIENTRY logTexCoord1dv (const GLdouble *v){

	fprintf(glwState.logFile, "glTexCoord1dv\n");
	dllTexCoord1dv(v);
}

static GLvoid APIENTRY logTexCoord1f (GLfloat s){

	fprintf(glwState.logFile, "glTexCoord1f\n");
	dllTexCoord1f(s);
}

static GLvoid APIENTRY logTexCoord1fv (const GLfloat *v){

	fprintf(glwState.logFile, "glTexCoord1fv\n");
	dllTexCoord1fv(v);
}

static GLvoid APIENTRY logTexCoord1i (GLint s){

	fprintf(glwState.logFile, "glTexCoord1i\n");
	dllTexCoord1i(s);
}

static GLvoid APIENTRY logTexCoord1iv (const GLint *v){

	fprintf(glwState.logFile, "glTexCoord1iv\n");
	dllTexCoord1iv(v);
}

static GLvoid APIENTRY logTexCoord1s (GLshort s){

	fprintf(glwState.logFile, "glTexCoord1s\n");
	dllTexCoord1s(s);
}

static GLvoid APIENTRY logTexCoord1sv (const GLshort *v){

	fprintf(glwState.logFile, "glTexCoord1sv\n");
	dllTexCoord1sv(v);
}

static GLvoid APIENTRY logTexCoord2d (GLdouble s, GLdouble t){

	fprintf(glwState.logFile, "glTexCoord2d\n");
	dllTexCoord2d(s, t);
}

static GLvoid APIENTRY logTexCoord2dv (const GLdouble *v){

	fprintf(glwState.logFile, "glTexCoord2dv\n");
	dllTexCoord2dv(v);
}

static GLvoid APIENTRY logTexCoord2f (GLfloat s, GLfloat t){

	fprintf(glwState.logFile, "glTexCoord2f\n");
	dllTexCoord2f(s, t);
}

static GLvoid APIENTRY logTexCoord2fv (const GLfloat *v){

	fprintf(glwState.logFile, "glTexCoord2fv\n");
	dllTexCoord2fv(v);
}

static GLvoid APIENTRY logTexCoord2i (GLint s, GLint t){

	fprintf(glwState.logFile, "glTexCoord2i\n");
	dllTexCoord2i(s, t);
}

static GLvoid APIENTRY logTexCoord2iv (const GLint *v){

	fprintf(glwState.logFile, "glTexCoord2iv\n");
	dllTexCoord2iv(v);
}

static GLvoid APIENTRY logTexCoord2s (GLshort s, GLshort t){

	fprintf(glwState.logFile, "glTexCoord2s\n");
	dllTexCoord2s(s, t);
}

static GLvoid APIENTRY logTexCoord2sv (const GLshort *v){

	fprintf(glwState.logFile, "glTexCoord2sv\n");
	dllTexCoord2sv(v);
}

static GLvoid APIENTRY logTexCoord3d (GLdouble s, GLdouble t, GLdouble r){

	fprintf(glwState.logFile, "glTexCoord3d\n");
	dllTexCoord3d(s, t, r);
}

static GLvoid APIENTRY logTexCoord3dv (const GLdouble *v){

	fprintf(glwState.logFile, "glTexCoord3dv\n");
	dllTexCoord3dv(v);
}

static GLvoid APIENTRY logTexCoord3f (GLfloat s, GLfloat t, GLfloat r){

	fprintf(glwState.logFile, "glTexCoord3f\n");
	dllTexCoord3f(s, t, r);
}

static GLvoid APIENTRY logTexCoord3fv (const GLfloat *v){

	fprintf(glwState.logFile, "glTexCoord3fv\n");
	dllTexCoord3fv(v);
}

static GLvoid APIENTRY logTexCoord3i (GLint s, GLint t, GLint r){

	fprintf(glwState.logFile, "glTexCoord3i\n");
	dllTexCoord3i(s, t, r);
}

static GLvoid APIENTRY logTexCoord3iv (const GLint *v){

	fprintf(glwState.logFile, "glTexCoord3iv\n");
	dllTexCoord3iv(v);
}

static GLvoid APIENTRY logTexCoord3s (GLshort s, GLshort t, GLshort r){

	fprintf(glwState.logFile, "glTexCoord3s\n");
	dllTexCoord3s(s, t, r);
}

static GLvoid APIENTRY logTexCoord3sv (const GLshort *v){

	fprintf(glwState.logFile, "glTexCoord3sv\n");
	dllTexCoord3sv(v);
}

static GLvoid APIENTRY logTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q){

	fprintf(glwState.logFile, "glTexCoord4d\n");
	dllTexCoord4d(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4dv (const GLdouble *v){

	fprintf(glwState.logFile, "glTexCoord4dv\n");
	dllTexCoord4dv(v);
}

static GLvoid APIENTRY logTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q){

	fprintf(glwState.logFile, "glTexCoord4f\n");
	dllTexCoord4f(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4fv (const GLfloat *v){

	fprintf(glwState.logFile, "glTexCoord4fv\n");
	dllTexCoord4fv(v);
}

static GLvoid APIENTRY logTexCoord4i (GLint s, GLint t, GLint r, GLint q){

	fprintf(glwState.logFile, "glTexCoord4i\n");
	dllTexCoord4i(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4iv (const GLint *v){

	fprintf(glwState.logFile, "glTexCoord4iv\n");
	dllTexCoord4iv(v);
}

static GLvoid APIENTRY logTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q){

	fprintf(glwState.logFile, "glTexCoord4s\n");
	dllTexCoord4s(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4sv (const GLshort *v){

	fprintf(glwState.logFile, "glTexCoord4sv\n");
	dllTexCoord4sv(v);
}

static GLvoid APIENTRY logTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glTexCoordPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllTexCoordPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logTexEnvf (GLenum target, GLenum pname, GLfloat param){

	char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";			break;
	default:						t = va("0x%X", target);			break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";		break;
	case GL_COMBINE_RGB_ARB:		n = "GL_COMBINE_RGB";			break;
	case GL_COMBINE_ALPHA_ARB:		n = "GL_COMBINE_ALPHA";			break;
	case GL_SOURCE0_RGB_ARB:		n = "GL_SOURCE0_RGB";			break;
	case GL_SOURCE1_RGB_ARB:		n = "GL_SOURCE1_RGB";			break;
	case GL_SOURCE2_RGB_ARB:		n = "GL_SOURCE2_RGB";			break;
	case GL_SOURCE0_ALPHA_ARB:		n = "GL_SOURCE0_ALPHA";			break;
	case GL_SOURCE1_ALPHA_ARB:		n = "GL_SOURCE1_ALPHA";			break;
	case GL_SOURCE2_ALPHA_ARB:		n = "GL_SOURCE2_ALPHA";			break;
	case GL_OPERAND0_RGB_ARB:		n = "GL_OPERAND0_RGB";			break;
	case GL_OPERAND1_RGB_ARB:		n = "GL_OPERAND1_RGB";			break;
	case GL_OPERAND2_RGB_ARB:		n = "GL_OPERAND2_RGB";			break;
	case GL_OPERAND0_ALPHA_ARB:		n = "GL_OPERAND0_ALPHA";		break;
	case GL_OPERAND1_ALPHA_ARB:		n = "GL_OPERAND1_ALPHA";		break;
	case GL_OPERAND2_ALPHA_ARB:		n = "GL_OPERAND2_ALPHA";		break;
	case GL_RGB_SCALE_ARB:			n = "GL_RGB_SCALE";				break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";			break;
	default:						n = va("0x%X", pname);			break;
	}

	switch ((int)param){
	case GL_MODULATE:				p = "GL_MODULATE";				break;
	case GL_DECAL:					p = "GL_DECAL";					break;
	case GL_BLEND:					p = "GL_BLEND";					break;
	case GL_REPLACE:				p = "GL_REPLACE";				break;
	case GL_ADD:					p = "GL_ADD";					break;
	case GL_TEXTURE:				p = "GL_TEXTURE";				break;
	case GL_SRC_COLOR:				p = "GL_SRC_COLOR";				break;
	case GL_ONE_MINUS_SRC_COLOR:	p = "GL_ONE_MINUS_SRC_COLOR";	break;
	case GL_SRC_ALPHA:				p = "GL_SRC_ALPHA";				break;
	case GL_ONE_MINUS_SRC_ALPHA:	p = "GL_ONE_MINUS_SRC_ALPHA";	break;
	case GL_COMBINE_ARB:			p = "GL_COMBINE";				break;
	case GL_ADD_SIGNED_ARB:			p = "GL_ADD_SIGNED";			break;
	case GL_INTERPOLATE_ARB:		p = "GL_INTERPOLATE";			break;
	case GL_SUBTRACT_ARB:			p = "GL_SUBTRACT";				break;
	case GL_CONSTANT_ARB:			p = "GL_CONSTANT";				break;
	case GL_PRIMARY_COLOR_ARB:		p = "GL_PRIMARY_COLOR";			break;
	case GL_PREVIOUS_ARB:			p = "GL_PREVIOUS";				break;
	case GL_DOT3_RGB_ARB:			p = "GL_DOT3_RGB";				break;
	case GL_DOT3_RGBA_ARB:			p = "GL_DOT3_RGBA";				break;
	default:						p = va("0x%X", param);			break;
	}

	if (pname == GL_RGB_SCALE_ARB || pname == GL_ALPHA_SCALE)
		p = va("%g", param);

	fprintf(glwState.logFile, "glTexEnvf( %s, %s, %s )\n", t, n, p);
	dllTexEnvf(target, pname, param);
}

static GLvoid APIENTRY logTexEnvfv (GLenum target, GLenum pname, const GLfloat *params){

	char	*t, *n;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";			break;
	default:						t = va("0x%X", target);			break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";		break;
	case GL_TEXTURE_ENV_COLOR:		n = "GL_TEXTURE_ENV_COLOR";		break;
	case GL_COMBINE_RGB_ARB:		n = "GL_COMBINE_RGB";			break;
	case GL_COMBINE_ALPHA_ARB:		n = "GL_COMBINE_ALPHA";			break;
	case GL_SOURCE0_RGB_ARB:		n = "GL_SOURCE0_RGB";			break;
	case GL_SOURCE1_RGB_ARB:		n = "GL_SOURCE1_RGB";			break;
	case GL_SOURCE2_RGB_ARB:		n = "GL_SOURCE2_RGB";			break;
	case GL_SOURCE0_ALPHA_ARB:		n = "GL_SOURCE0_ALPHA";			break;
	case GL_SOURCE1_ALPHA_ARB:		n = "GL_SOURCE1_ALPHA";			break;
	case GL_SOURCE2_ALPHA_ARB:		n = "GL_SOURCE2_ALPHA";			break;
	case GL_OPERAND0_RGB_ARB:		n = "GL_OPERAND0_RGB";			break;
	case GL_OPERAND1_RGB_ARB:		n = "GL_OPERAND1_RGB";			break;
	case GL_OPERAND2_RGB_ARB:		n = "GL_OPERAND2_RGB";			break;
	case GL_OPERAND0_ALPHA_ARB:		n = "GL_OPERAND0_ALPHA";		break;
	case GL_OPERAND1_ALPHA_ARB:		n = "GL_OPERAND1_ALPHA";		break;
	case GL_OPERAND2_ALPHA_ARB:		n = "GL_OPERAND2_ALPHA";		break;
	case GL_RGB_SCALE_ARB:			n = "GL_RGB_SCALE";				break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";			break;
	default:						n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glTexEnvfv( %s, %s, %p )\n", t, n, params);
	dllTexEnvfv(target, pname, params);
}

static GLvoid APIENTRY logTexEnvi (GLenum target, GLenum pname, GLint param){

	char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";			break;
	default:						t = va("0x%X", target);			break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";		break;
	case GL_COMBINE_RGB_ARB:		n = "GL_COMBINE_RGB";			break;
	case GL_COMBINE_ALPHA_ARB:		n = "GL_COMBINE_ALPHA";			break;
	case GL_SOURCE0_RGB_ARB:		n = "GL_SOURCE0_RGB";			break;
	case GL_SOURCE1_RGB_ARB:		n = "GL_SOURCE1_RGB";			break;
	case GL_SOURCE2_RGB_ARB:		n = "GL_SOURCE2_RGB";			break;
	case GL_SOURCE0_ALPHA_ARB:		n = "GL_SOURCE0_ALPHA";			break;
	case GL_SOURCE1_ALPHA_ARB:		n = "GL_SOURCE1_ALPHA";			break;
	case GL_SOURCE2_ALPHA_ARB:		n = "GL_SOURCE2_ALPHA";			break;
	case GL_OPERAND0_RGB_ARB:		n = "GL_OPERAND0_RGB";			break;
	case GL_OPERAND1_RGB_ARB:		n = "GL_OPERAND1_RGB";			break;
	case GL_OPERAND2_RGB_ARB:		n = "GL_OPERAND2_RGB";			break;
	case GL_OPERAND0_ALPHA_ARB:		n = "GL_OPERAND0_ALPHA";		break;
	case GL_OPERAND1_ALPHA_ARB:		n = "GL_OPERAND1_ALPHA";		break;
	case GL_OPERAND2_ALPHA_ARB:		n = "GL_OPERAND2_ALPHA";		break;
	case GL_RGB_SCALE_ARB:			n = "GL_RGB_SCALE";				break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";			break;
	default:						n = va("0x%X", pname);			break;
	}

	switch (param){
	case GL_MODULATE:				p = "GL_MODULATE";				break;
	case GL_DECAL:					p = "GL_DECAL";					break;
	case GL_BLEND:					p = "GL_BLEND";					break;
	case GL_REPLACE:				p = "GL_REPLACE";				break;
	case GL_ADD:					p = "GL_ADD";					break;
	case GL_TEXTURE:				p = "GL_TEXTURE";				break;
	case GL_SRC_COLOR:				p = "GL_SRC_COLOR";				break;
	case GL_ONE_MINUS_SRC_COLOR:	p = "GL_ONE_MINUS_SRC_COLOR";	break;
	case GL_SRC_ALPHA:				p = "GL_SRC_ALPHA";				break;
	case GL_ONE_MINUS_SRC_ALPHA:	p = "GL_ONE_MINUS_SRC_ALPHA";	break;
	case GL_COMBINE_ARB:			p = "GL_COMBINE";				break;
	case GL_ADD_SIGNED_ARB:			p = "GL_ADD_SIGNED";			break;
	case GL_INTERPOLATE_ARB:		p = "GL_INTERPOLATE";			break;
	case GL_SUBTRACT_ARB:			p = "GL_SUBTRACT";				break;
	case GL_CONSTANT_ARB:			p = "GL_CONSTANT";				break;
	case GL_PRIMARY_COLOR_ARB:		p = "GL_PRIMARY_COLOR";			break;
	case GL_PREVIOUS_ARB:			p = "GL_PREVIOUS";				break;
	case GL_DOT3_RGB_ARB:			p = "GL_DOT3_RGB";				break;
	case GL_DOT3_RGBA_ARB:			p = "GL_DOT3_RGBA";				break;
	default:						p = va("0x%X", param);			break;
	}

	if (pname == GL_RGB_SCALE_ARB || pname == GL_ALPHA_SCALE)
		p = va("%i", param);

	fprintf(glwState.logFile, "glTexEnvi( %s, %s, %s )\n", t, n, p);
	dllTexEnvi(target, pname, param);
}

static GLvoid APIENTRY logTexEnviv (GLenum target, GLenum pname, const GLint *params){

	char	*t, *n;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";			break;
	default:						t = va("0x%X", target);			break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";		break;
	case GL_TEXTURE_ENV_COLOR:		n = "GL_TEXTURE_ENV_COLOR";		break;
	case GL_COMBINE_RGB_ARB:		n = "GL_COMBINE_RGB";			break;
	case GL_COMBINE_ALPHA_ARB:		n = "GL_COMBINE_ALPHA";			break;
	case GL_SOURCE0_RGB_ARB:		n = "GL_SOURCE0_RGB";			break;
	case GL_SOURCE1_RGB_ARB:		n = "GL_SOURCE1_RGB";			break;
	case GL_SOURCE2_RGB_ARB:		n = "GL_SOURCE2_RGB";			break;
	case GL_SOURCE0_ALPHA_ARB:		n = "GL_SOURCE0_ALPHA";			break;
	case GL_SOURCE1_ALPHA_ARB:		n = "GL_SOURCE1_ALPHA";			break;
	case GL_SOURCE2_ALPHA_ARB:		n = "GL_SOURCE2_ALPHA";			break;
	case GL_OPERAND0_RGB_ARB:		n = "GL_OPERAND0_RGB";			break;
	case GL_OPERAND1_RGB_ARB:		n = "GL_OPERAND1_RGB";			break;
	case GL_OPERAND2_RGB_ARB:		n = "GL_OPERAND2_RGB";			break;
	case GL_OPERAND0_ALPHA_ARB:		n = "GL_OPERAND0_ALPHA";		break;
	case GL_OPERAND1_ALPHA_ARB:		n = "GL_OPERAND1_ALPHA";		break;
	case GL_OPERAND2_ALPHA_ARB:		n = "GL_OPERAND2_ALPHA";		break;
	case GL_RGB_SCALE_ARB:			n = "GL_RGB_SCALE";				break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";			break;
	default:						n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glTexEnviv( %s, %s, %p )\n", t, n, params);
	dllTexEnviv(target, pname, params);
}

static GLvoid APIENTRY logTexGend (GLenum coord, GLenum pname, GLdouble param){

	char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	default:					n = va("0x%X", pname);		break;
	}

	switch ((int)param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";		break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";		break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";		break;
	case GL_REFLECTION_MAP_ARB:	p = "GL_REFLECTION_MAP";	break;
	case GL_NORMAL_MAP_ARB:		p = "GL_NORMAL_MAP";		break;
	default:					p = va("0x%X", param);		break;
	}

	fprintf(glwState.logFile, "glTexGend( %s, %s, %s )\n", c, n, p);
	dllTexGend(coord, pname, param);
}

static GLvoid APIENTRY logTexGendv (GLenum coord, GLenum pname, const GLdouble *params){

	char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";		break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";			break;
	default:					n = va("0x%X", pname);		break;
	}

	fprintf(glwState.logFile, "glTexGendv( %s, %s, %p )\n", c, n, params);
	dllTexGendv(coord, pname, params);
}

static GLvoid APIENTRY logTexGenf (GLenum coord, GLenum pname, GLfloat param){

	char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	default:					n = va("0x%X", pname);		break;
	}

	switch ((int)param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";		break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";		break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";		break;
	case GL_REFLECTION_MAP_ARB:	p = "GL_REFLECTION_MAP";	break;
	case GL_NORMAL_MAP_ARB:		p = "GL_NORMAL_MAP";		break;
	default:					p = va("0x%X", param);		break;
	}

	fprintf(glwState.logFile, "glTexGenf( %s, %s, %s )\n", c, n, p);
	dllTexGenf(coord, pname, param);
}

static GLvoid APIENTRY logTexGenfv (GLenum coord, GLenum pname, const GLfloat *params){

	char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";		break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";			break;
	default:					n = va("0x%X", pname);		break;
	}

	fprintf(glwState.logFile, "glTexGenfv( %s, %s, %p )\n", c, n, params);
	dllTexGenfv(coord, pname, params);
}

static GLvoid APIENTRY logTexGeni (GLenum coord, GLenum pname, GLint param){

	char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	default:					n = va("0x%X", pname);		break;
	}

	switch (param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";		break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";		break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";		break;
	case GL_REFLECTION_MAP_ARB:	p = "GL_REFLECTION_MAP";	break;
	case GL_NORMAL_MAP_ARB:		p = "GL_NORMAL_MAP";		break;
	default:					p = va("0x%X", param);		break;
	}

	fprintf(glwState.logFile, "glTexGeni( %s, %s, %s )\n", c, n, p);
	dllTexGeni(coord, pname, param);
}

static GLvoid APIENTRY logTexGeniv (GLenum coord, GLenum pname, const GLint *params){

	char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";					break;
	case GL_T:					c = "GL_T";					break;
	case GL_R:					c = "GL_R";					break;
	case GL_Q:					c = "GL_Q";					break;
	default:					c = va("0x%X", coord);		break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";	break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";		break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";			break;
	default:					n = va("0x%X", pname);		break;
	}

	fprintf(glwState.logFile, "glTexGeniv( %s, %s, %p )\n", c, n, params);
	dllTexGeniv(coord, pname, params);
}

static GLvoid APIENTRY logTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels){

	char	*t, *i, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D:						t = "GL_TEXTURE_1D";					break;
	default:								t = va("0x%X", target);					break;
	}

	switch (internalformat){
	case GL_ALPHA:							i = "GL_ALPHA";							break;
	case GL_ALPHA4:							i = "GL_ALPHA4";						break;
	case GL_ALPHA8:							i = "GL_ALPHA8";						break;
	case GL_ALPHA12:						i = "GL_ALPHA12";						break;
	case GL_ALPHA16:						i = "GL_ALPHA16";						break;
	case GL_LUMINANCE:						i = "GL_LUMINANCE";						break;
	case GL_LUMINANCE4:						i = "GL_LUMINANCE4";					break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";					break;
	case GL_LUMINANCE12:					i = "GL_LUMINANCE12";					break;
	case GL_LUMINANCE16:					i = "GL_LUMINANCE16";					break;
	case GL_LUMINANCE_ALPHA:				i = "GL_LUMINANCE_ALPHA";				break;
	case GL_LUMINANCE4_ALPHA4:				i = "GL_LUMINANCE4_ALPHA4";				break;
	case GL_LUMINANCE6_ALPHA2:				i = "GL_LUMINANCE6_ALPHA2";				break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";				break;
	case GL_LUMINANCE12_ALPHA4:				i = "GL_LUMINANCE12_ALPHA4";			break;
	case GL_LUMINANCE12_ALPHA12:			i = "GL_LUMINANCE12_ALPHA12";			break;
	case GL_LUMINANCE16_ALPHA16:			i = "GL_LUMINANCE16_ALPHA16";			break;
	case GL_INTENSITY:						i = "GL_INTENSITY";						break;
	case GL_INTENSITY4:						i = "GL_INTENSITY4";					break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";					break;
	case GL_INTENSITY12:					i = "GL_INTENSITY12";					break;
	case GL_INTENSITY16:					i = "GL_INTENSITY16";					break;
	case GL_R3_G3_B2:						i = "GL_R3_G3_B2";						break;
	case GL_RGB:							i = "GL_RGB";							break;
	case GL_RGB4:							i = "GL_RGB4";							break;
	case GL_RGB5:							i = "GL_RGB5";							break;
	case GL_RGB8:							i = "GL_RGB8";							break;
	case GL_RGB10:							i = "GL_RGB10";							break;
	case GL_RGB12:							i = "GL_RGB12";							break;
	case GL_RGB16:							i = "GL_RGB16";							break;
	case GL_RGBA:							i = "GL_RGBA";							break;
	case GL_RGBA2:							i = "GL_RGBA2";							break;
	case GL_RGBA4:							i = "GL_RGBA4";							break;
	case GL_RGB5_A1:						i = "GL_RGB5_A1";						break;
	case GL_RGBA8:							i = "GL_RGBA8";							break;
	case GL_RGB10_A2:						i = "GL_RGB10_A2";						break;
	case GL_RGBA12:							i = "GL_RGBA12";						break;
	case GL_RGBA16:							i = "GL_RGBA16";						break;
	case GL_COMPRESSED_ALPHA_ARB:			i = "GL_COMPRESSED_ALPHA";				break;
	case GL_COMPRESSED_RGB_ARB:				i = "GL_COMPRESSED_RGB";				break;
	case GL_COMPRESSED_RGBA_ARB:			i = "GL_COMPRESSED_RGBA";				break;
	case GL_COMPRESSED_LUMINANCE_ARB:		i = "GL_COMPRESSED_LUMINANCE";			break;
	case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:	i = "GL_COMPRESSED_LUMINANCE_ALPHA";	break;
	case GL_COMPRESSED_INTENSITY_ARB:		i = "GL_COMPRESSED_INTENSITY";			break;
	default:								i = va("0x%X", internalformat);			break;
	}

	switch (format){
	case GL_COLOR_INDEX:					f = "GL_COLOR_INDEX";					break;
	case GL_RED:							f = "GL_RED";							break;
	case GL_GREEN:							f = "GL_GREEN";							break;
	case GL_BLUE:							f = "GL_BLUE";							break;
	case GL_ALPHA:							f = "GL_ALPHA";							break;
	case GL_RGB:							f = "GL_RGB";							break;
	case GL_RGBA:							f = "GL_RGBA";							break;
	case GL_BGR:							f = "GL_BGR";							break;
	case GL_BGRA:							f = "GL_BGRA";							break;
	case GL_LUMINANCE:						f = "GL_LUMINANCE";						break;
	case GL_LUMINANCE_ALPHA:				f = "GL_LUMINANCE_ALPHA";				break;
	default:								f = va("0x%X", format);					break;
	}

	switch (type){
	case GL_BYTE:							t2 = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:					t2 = "GL_UNSIGNED_BYTE";				break;
	case GL_BITMAP:							t2 = "GL_BITMAP";						break;
	case GL_SHORT:							t2 = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:					t2 = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:							t2 = "GL_INT";							break;
	case GL_UNSIGNED_INT:					t2 = "GL_UNSIGNED_INT";					break;
	case GL_FLOAT:							t2 = "GL_FLOAT";						break;
	default:								t2 = va("0x%X", type);					break;
	}

	fprintf(glwState.logFile, "glTexImage1D( %s, %i, %s, %i, %i, %s, %s, %p )\n", t, level, i, width, border, f, t2, pixels);
	dllTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

static GLvoid APIENTRY logTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels){

	char	*t, *i, *f, *t2;

	switch (target){
	case GL_TEXTURE_2D:							t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	default:									t = va("0x%X", target);					break;
	}

	switch (internalformat){
	case GL_ALPHA:								i = "GL_ALPHA";							break;
	case GL_ALPHA4:								i = "GL_ALPHA4";						break;
	case GL_ALPHA8:								i = "GL_ALPHA8";						break;
	case GL_ALPHA12:							i = "GL_ALPHA12";						break;
	case GL_ALPHA16:							i = "GL_ALPHA16";						break;
	case GL_LUMINANCE:							i = "GL_LUMINANCE";						break;
	case GL_LUMINANCE4:							i = "GL_LUMINANCE4";					break;
	case GL_LUMINANCE8:							i = "GL_LUMINANCE8";					break;
	case GL_LUMINANCE12:						i = "GL_LUMINANCE12";					break;
	case GL_LUMINANCE16:						i = "GL_LUMINANCE16";					break;
	case GL_LUMINANCE_ALPHA:					i = "GL_LUMINANCE_ALPHA";				break;
	case GL_LUMINANCE4_ALPHA4:					i = "GL_LUMINANCE4_ALPHA4";				break;
	case GL_LUMINANCE6_ALPHA2:					i = "GL_LUMINANCE6_ALPHA2";				break;
	case GL_LUMINANCE8_ALPHA8:					i = "GL_LUMINANCE8_ALPHA8";				break;
	case GL_LUMINANCE12_ALPHA4:					i = "GL_LUMINANCE12_ALPHA4";			break;
	case GL_LUMINANCE12_ALPHA12:				i = "GL_LUMINANCE12_ALPHA12";			break;
	case GL_LUMINANCE16_ALPHA16:				i = "GL_LUMINANCE16_ALPHA16";			break;
	case GL_INTENSITY:							i = "GL_INTENSITY";						break;
	case GL_INTENSITY4:							i = "GL_INTENSITY4";					break;
	case GL_INTENSITY8:							i = "GL_INTENSITY8";					break;
	case GL_INTENSITY12:						i = "GL_INTENSITY12";					break;
	case GL_INTENSITY16:						i = "GL_INTENSITY16";					break;
	case GL_R3_G3_B2:							i = "GL_R3_G3_B2";						break;
	case GL_RGB:								i = "GL_RGB";							break;
	case GL_RGB4:								i = "GL_RGB4";							break;
	case GL_RGB5:								i = "GL_RGB5";							break;
	case GL_RGB8:								i = "GL_RGB8";							break;
	case GL_RGB10:								i = "GL_RGB10";							break;
	case GL_RGB12:								i = "GL_RGB12";							break;
	case GL_RGB16:								i = "GL_RGB16";							break;
	case GL_RGBA:								i = "GL_RGBA";							break;
	case GL_RGBA2:								i = "GL_RGBA2";							break;
	case GL_RGBA4:								i = "GL_RGBA4";							break;
	case GL_RGB5_A1:							i = "GL_RGB5_A1";						break;
	case GL_RGBA8:								i = "GL_RGBA8";							break;
	case GL_RGB10_A2:							i = "GL_RGB10_A2";						break;
	case GL_RGBA12:								i = "GL_RGBA12";						break;
	case GL_RGBA16:								i = "GL_RGBA16";						break;
	case GL_COMPRESSED_ALPHA_ARB:				i = "GL_COMPRESSED_ALPHA";				break;
	case GL_COMPRESSED_RGB_ARB:					i = "GL_COMPRESSED_RGB";				break;
	case GL_COMPRESSED_RGBA_ARB:				i = "GL_COMPRESSED_RGBA";				break;
	case GL_COMPRESSED_LUMINANCE_ARB:			i = "GL_COMPRESSED_LUMINANCE";			break;
	case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";	break;
	case GL_COMPRESSED_INTENSITY_ARB:			i = "GL_COMPRESSED_INTENSITY";			break;
	default:									i = va("0x%X", internalformat);			break;
	}

	switch (format){
	case GL_COLOR_INDEX:						f = "GL_COLOR_INDEX";					break;
	case GL_RED:								f = "GL_RED";							break;
	case GL_GREEN:								f = "GL_GREEN";							break;
	case GL_BLUE:								f = "GL_BLUE";							break;
	case GL_ALPHA:								f = "GL_ALPHA";							break;
	case GL_RGB:								f = "GL_RGB";							break;
	case GL_RGBA:								f = "GL_RGBA";							break;
	case GL_BGR:								f = "GL_BGR";							break;
	case GL_BGRA:								f = "GL_BGRA";							break;
	case GL_LUMINANCE:							f = "GL_LUMINANCE";						break;
	case GL_LUMINANCE_ALPHA:					f = "GL_LUMINANCE_ALPHA";				break;
	default:									f = va("0x%X", format);					break;
	}

	switch (type){
	case GL_BYTE:								t2 = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:						t2 = "GL_UNSIGNED_BYTE";				break;
	case GL_BITMAP:								t2 = "GL_BITMAP";						break;
	case GL_SHORT:								t2 = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:						t2 = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:								t2 = "GL_INT";							break;
	case GL_UNSIGNED_INT:						t2 = "GL_UNSIGNED_INT";					break;
	case GL_FLOAT:								t2 = "GL_FLOAT";						break;
	default:									t2 = va("0x%X", type);					break;
	}

	fprintf(glwState.logFile, "glTexImage2D( %s, %i, %s, %i, %i, %i, %s, %s, %p )\n", t, level, i, width, height, border, f, t2, pixels);
	dllTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

static GLvoid APIENTRY logTexParameterf (GLenum target, GLenum pname, GLfloat param){

	char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP_ARB:		t = "GL_TEXTURE_CUBE_MAP";			break;
	default:							t = va("0x%X", target);				break;
	}

	switch (pname){
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	case GL_TEXTURE_LOD_BIAS_EXT:		n = "GL_TEXTURE_LOD_BIAS";			break;
	default:							n = va("0x%X", pname);				break;
	}

	switch ((int)param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER_ARB:		p = "GL_CLAMP_TO_BORDER";			break;
	default:							p = va("0x%X", param);				break;
	}

	if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT || pname == GL_TEXTURE_LOD_BIAS_EXT)
		p = va("%g", param);

	fprintf(glwState.logFile, "glTexParameterf( %s, %s, %s )\n", t, n, p);
	dllTexParameterf(target, pname, param);
}

static GLvoid APIENTRY logTexParameterfv (GLenum target, GLenum pname, const GLfloat *params){

	char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP_ARB:		t = "GL_TEXTURE_CUBE_MAP";			break;
	default:							t = va("0x%X", target);				break;
	}

	switch (pname){
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	case GL_TEXTURE_LOD_BIAS_EXT:		n = "GL_TEXTURE_LOD_BIAS";			break;
	default:							n = va("0x%X", pname);				break;
	}

	fprintf(glwState.logFile, "glTexParameterfv( %s, %s, %p )\n", t, n, params);
	dllTexParameterfv(target, pname, params);
}

static GLvoid APIENTRY logTexParameteri (GLenum target, GLenum pname, GLint param){

	char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP_ARB:		t = "GL_TEXTURE_CUBE_MAP";			break;
	default:							t = va("0x%X", target);				break;
	}

	switch (pname){
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	case GL_TEXTURE_LOD_BIAS_EXT:		n = "GL_TEXTURE_LOD_BIAS";			break;
	default:							n = va("0x%X", pname);				break;
	}

	switch (param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER_ARB:		p = "GL_CLAMP_TO_BORDER";			break;
	default:							p = va("0x%X", param);				break;
	}

	if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT || pname == GL_TEXTURE_LOD_BIAS_EXT)
		p = va("%i", param);

	fprintf(glwState.logFile, "glTexParameteri( %s, %s, %s )\n", t, n, p);
	dllTexParameteri(target, pname, param);
}

static GLvoid APIENTRY logTexParameteriv (GLenum target, GLenum pname, const GLint *params){

	char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP_ARB:		t = "GL_TEXTURE_CUBE_MAP";			break;
	default:							t = va("0x%X", target);				break;
	}

	switch (pname){
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	case GL_TEXTURE_LOD_BIAS_EXT:		n = "GL_TEXTURE_LOD_BIAS";			break;
	default:							n = va("0x%X", pname);				break;
	}

	fprintf(glwState.logFile, "glTexParameteriv( %s, %s, %p )\n", t, n, params);
	dllTexParameteriv(target, pname, params);
}

static GLvoid APIENTRY logTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels){

	char	*t, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D:			t = "GL_TEXTURE_1D";		break;
	default:					t = va("0x%X", target);		break;
	}

	switch (format){
	case GL_COLOR_INDEX:		f = "GL_COLOR_INDEX";		break;
	case GL_RED:				f = "GL_RED";				break;
	case GL_GREEN:				f = "GL_GREEN";				break;
	case GL_BLUE:				f = "GL_BLUE";				break;
	case GL_ALPHA:				f = "GL_ALPHA";				break;
	case GL_RGB:				f = "GL_RGB";				break;
	case GL_RGBA:				f = "GL_RGBA";				break;
	case GL_BGR:				f = "GL_BGR";				break;
	case GL_BGRA:				f = "GL_BGRA";				break;
	case GL_LUMINANCE:			f = "GL_LUMINANCE";			break;
	case GL_LUMINANCE_ALPHA:	f = "GL_LUMINANCE_ALPHA";	break;
	default:					f = va("0x%X", format);		break;
	}

	switch (type){
	case GL_BYTE:				t2 = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:		t2 = "GL_UNSIGNED_BYTE";	break;
	case GL_BITMAP:				t2 = "GL_BITMAP";			break;
	case GL_SHORT:				t2 = "GL_SHORT";			break;
	case GL_UNSIGNED_SHORT:		t2 = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:				t2 = "GL_INT";				break;
	case GL_UNSIGNED_INT:		t2 = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:				t2 = "GL_FLOAT";			break;
	default:					t2 = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glTexSubImage1D( %s, %i, %i, %i, %s, %s, %p )\n", t, level, xoffset, width, f, t2, pixels);
	dllTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

static GLvoid APIENTRY logTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){

	char	*t, *f, *t2;

	switch (target){
	case GL_TEXTURE_2D:							t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	default:									t = va("0x%X", target);					break;
	}

	switch (format){
	case GL_COLOR_INDEX:						f = "GL_COLOR_INDEX";					break;
	case GL_RED:								f = "GL_RED";							break;
	case GL_GREEN:								f = "GL_GREEN";							break;
	case GL_BLUE:								f = "GL_BLUE";							break;
	case GL_ALPHA:								f = "GL_ALPHA";							break;
	case GL_RGB:								f = "GL_RGB";							break;
	case GL_RGBA:								f = "GL_RGBA";							break;
	case GL_BGR:								f = "GL_BGR";							break;
	case GL_BGRA:								f = "GL_BGRA";							break;
	case GL_LUMINANCE:							f = "GL_LUMINANCE";						break;
	case GL_LUMINANCE_ALPHA:					f = "GL_LUMINANCE_ALPHA";				break;
	default:									f = va("0x%X", format);					break;
	}

	switch (type){
	case GL_BYTE:								t2 = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:						t2 = "GL_UNSIGNED_BYTE";				break;
	case GL_BITMAP:								t2 = "GL_BITMAP";						break;
	case GL_SHORT:								t2 = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:						t2 = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:								t2 = "GL_INT";							break;
	case GL_UNSIGNED_INT:						t2 = "GL_UNSIGNED_INT";					break;
	case GL_FLOAT:								t2 = "GL_FLOAT";						break;
	default:									t2 = va("0x%X", type);					break;
	}

	fprintf(glwState.logFile, "glTexSubImage2D( %s, %i, %i, %i, %i, %i, %s, %s, %p )\n", t, level, xoffset, yoffset, width, height, f, t2, pixels);
	dllTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static GLvoid APIENTRY logTranslated (GLdouble x, GLdouble y, GLdouble z){

	fprintf(glwState.logFile, "glTranslated\n");
	dllTranslated(x, y, z);
}

static GLvoid APIENTRY logTranslatef (GLfloat x, GLfloat y, GLfloat z){

	fprintf(glwState.logFile, "glTranslatef\n");
	dllTranslatef(x, y, z);
}

static GLvoid APIENTRY logVertex2d (GLdouble x, GLdouble y){

	fprintf(glwState.logFile, "glVertex2d\n");
	dllVertex2d(x, y);
}

static GLvoid APIENTRY logVertex2dv (const GLdouble *v){

	fprintf(glwState.logFile, "glVertex2dv\n");
	dllVertex2dv(v);
}

static GLvoid APIENTRY logVertex2f (GLfloat x, GLfloat y){

	fprintf(glwState.logFile, "glVertex2f\n");
	dllVertex2f(x, y);
}

static GLvoid APIENTRY logVertex2fv (const GLfloat *v){

	fprintf(glwState.logFile, "glVertex2fv\n");
	dllVertex2fv(v);
}

static GLvoid APIENTRY logVertex2i (GLint x, GLint y){

	fprintf(glwState.logFile, "glVertex2i\n");
	dllVertex2i(x, y);
}

static GLvoid APIENTRY logVertex2iv (const GLint *v){

	fprintf(glwState.logFile, "glVertex2iv\n");
	dllVertex2iv(v);
}

static GLvoid APIENTRY logVertex2s (GLshort x, GLshort y){

	fprintf(glwState.logFile, "glVertex2s\n");
	dllVertex2s(x, y);
}

static GLvoid APIENTRY logVertex2sv (const GLshort *v){

	fprintf(glwState.logFile, "glVertex2sv\n");
	dllVertex2sv(v);
}

static GLvoid APIENTRY logVertex3d (GLdouble x, GLdouble y, GLdouble z){

	fprintf(glwState.logFile, "glVertex3d\n");
	dllVertex3d(x, y, z);
}

static GLvoid APIENTRY logVertex3dv (const GLdouble *v){

	fprintf(glwState.logFile, "glVertex3dv\n");
	dllVertex3dv(v);
}

static GLvoid APIENTRY logVertex3f (GLfloat x, GLfloat y, GLfloat z){

	fprintf(glwState.logFile, "glVertex3f\n");
	dllVertex3f(x, y, z);
}

static GLvoid APIENTRY logVertex3fv (const GLfloat *v){

	fprintf(glwState.logFile, "glVertex3fv\n");
	dllVertex3fv(v);
}

static GLvoid APIENTRY logVertex3i (GLint x, GLint y, GLint z){

	fprintf(glwState.logFile, "glVertex3i\n");
	dllVertex3i(x, y, z);
}

static GLvoid APIENTRY logVertex3iv (const GLint *v){

	fprintf(glwState.logFile, "glVertex3iv\n");
	dllVertex3iv(v);
}

static GLvoid APIENTRY logVertex3s (GLshort x, GLshort y, GLshort z){

	fprintf(glwState.logFile, "glVertex3s\n");
	dllVertex3s(x, y, z);
}

static GLvoid APIENTRY logVertex3sv (const GLshort *v){

	fprintf(glwState.logFile, "glVertex3sv\n");
	dllVertex3sv(v);
}

static GLvoid APIENTRY logVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w){

	fprintf(glwState.logFile, "glVertex4d\n");
	dllVertex4d(x, y, z, w);
}

static GLvoid APIENTRY logVertex4dv (const GLdouble *v){

	fprintf(glwState.logFile, "glVertex4dv\n");
	dllVertex4dv(v);
}

static GLvoid APIENTRY logVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	fprintf(glwState.logFile, "glVertex4f\n");
	dllVertex4f(x, y, z, w);
}

static GLvoid APIENTRY logVertex4fv (const GLfloat *v){

	fprintf(glwState.logFile, "glVertex4fv\n");
	dllVertex4fv(v);
}

static GLvoid APIENTRY logVertex4i (GLint x, GLint y, GLint z, GLint w){

	fprintf(glwState.logFile, "glVertex4i\n");
	dllVertex4i(x, y, z, w);
}

static GLvoid APIENTRY logVertex4iv (const GLint *v){

	fprintf(glwState.logFile, "glVertex4iv\n");
	dllVertex4iv(v);
}

static GLvoid APIENTRY logVertex4s (GLshort x, GLshort y, GLshort z, GLshort w){

	fprintf(glwState.logFile, "glVertex4s\n");
	dllVertex4s(x, y, z, w);
}

static GLvoid APIENTRY logVertex4sv (const GLshort *v){

	fprintf(glwState.logFile, "glVertex4sv\n");
	dllVertex4sv(v);
}

static GLvoid APIENTRY logVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glVertexPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllVertexPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logViewport (GLint x, GLint y, GLsizei width, GLsizei height){

	fprintf(glwState.logFile, "glViewport( %i, %i, %i, %i )\n", x, y, width, height);
	dllViewport(x, y, width, height);
}

static GLvoid APIENTRY logActiveTextureARB (GLenum texture){

	char	*t;

	if (texture >= GL_TEXTURE0_ARB && texture <= GL_TEXTURE31_ARB)
		t = va("GL_TEXTURE%i", texture - GL_TEXTURE0_ARB);
	else
		t = va("0x%X", texture);

	fprintf(glwState.logFile, "glActiveTexture( %s )\n", t);
	dllActiveTextureARB(texture);
}

static GLvoid APIENTRY logClientActiveTextureARB (GLenum texture){

	char	*t;

	if (texture >= GL_TEXTURE0_ARB && texture <= GL_TEXTURE31_ARB)
		t = va("GL_TEXTURE%i", texture - GL_TEXTURE0_ARB);
	else
		t = va("0x%X", texture);

	fprintf(glwState.logFile, "glClientActiveTexture( %s )\n", t);
	dllClientActiveTextureARB(texture);
}

static GLvoid APIENTRY logBindBufferARB (GLenum target, GLuint buffer){

	char	*t;

	switch (target){
	case GL_ARRAY_BUFFER_ARB:			t = "GL_ARRAY_BUFFER";			break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:	t = "GL_ELEMENT_ARRAY_BUFFER";	break;
	default:							t = va("0x%X", target);			break;
	}

	fprintf(glwState.logFile, "glBindBuffer( %s, %u )\n", t, buffer);
	dllBindBufferARB(target, buffer);
}

static GLvoid APIENTRY logDeleteBuffersARB (GLsizei n, const GLuint *buffers){

	fprintf(glwState.logFile, "glDeleteBuffers\n");
	dllDeleteBuffersARB(n, buffers);
}

static GLvoid APIENTRY logGenBuffersARB (GLsizei n, GLuint *buffers){

	fprintf(glwState.logFile, "glGenBuffers\n");
	dllGenBuffersARB(n, buffers);
}

static GLvoid APIENTRY logBufferDataARB (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage){

	char	*t, *u;

	switch (target){
	case GL_ARRAY_BUFFER_ARB:			t = "GL_ARRAY_BUFFER";			break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:	t = "GL_ELEMENT_ARRAY_BUFFER";	break;
	default:							t = va("0x%X", target);			break;
	}

	switch (usage){
	case GL_STATIC_DRAW_ARB:			u = "GL_STATIC_DRAW";			break;
	case GL_STATIC_READ_ARB:			u = "GL_STATIC_READ";			break;
	case GL_STATIC_COPY_ARB:			u = "GL_STATIC_COPY";			break;
	case GL_DYNAMIC_DRAW_ARB:			u = "GL_DYNAMIC_DRAW";			break;
	case GL_DYNAMIC_READ_ARB:			u = "GL_DYNAMIC_READ";			break;
	case GL_DYNAMIC_COPY_ARB:			u = "GL_DYNAMIC_COPY";			break;
	case GL_STREAM_DRAW_ARB:			u = "GL_STREAM_DRAW";			break;
	case GL_STREAM_READ_ARB:			u = "GL_STREAM_READ";			break;
	case GL_STREAM_COPY_ARB:			u = "GL_STREAM_COPY";			break;
	default:							u = va("0x%X", usage);			break;
	}

	fprintf(glwState.logFile, "glBufferData( %s, %i, %p, %s )\n", t, size, data, u);
	dllBufferDataARB(target, size, data, usage);
}

static GLvoid APIENTRY logBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data){

	char	*t;

	switch (target){
	case GL_ARRAY_BUFFER_ARB:			t = "GL_ARRAY_BUFFER";			break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:	t = "GL_ELEMENT_ARRAY_BUFFER";	break;
	default:							t = va("0x%X", target);			break;
	}

	fprintf(glwState.logFile, "glBufferSubData( %s, %i, %i, %p )\n", t, offset, size, data);
	dllBufferSubDataARB(target, offset, size, data);
}

static GLvoid * APIENTRY logMapBufferARB (GLenum target, GLenum access){

	char	*t, *a;

	switch (target){
	case GL_ARRAY_BUFFER_ARB:			t = "GL_ARRAY_BUFFER";			break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:	t = "GL_ELEMENT_ARRAY_BUFFER";	break;
	default:							t = va("0x%X", target);			break;
	}

	switch (access){
	case GL_READ_ONLY_ARB:				a = "GL_READ_ONLY";				break;
	case GL_WRITE_ONLY_ARB:				a = "GL_WRITE_ONLY";			break;
	case GL_READ_WRITE_ARB:				a = "GL_READ_WRITE";			break;
	default:							a = va("0x%X", access);			break;
	}

	fprintf(glwState.logFile, "glMapBuffer( %s, %s )\n", t, a);
	return dllMapBufferARB(target, access);
}

static GLboolean APIENTRY logUnmapBufferARB (GLenum target){

	char	*t;

	switch (target){
	case GL_ARRAY_BUFFER_ARB:			t = "GL_ARRAY_BUFFER";			break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:	t = "GL_ELEMENT_ARRAY_BUFFER";	break;
	default:							t = va("0x%X", target);			break;
	}

	fprintf(glwState.logFile, "glUnmapBuffer( %s )\n", t);
	return dllUnmapBufferARB(target);
}

static GLvoid APIENTRY logVertexAttribPointerARB (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer){

	char	*t, *n;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	switch (normalized){
	case GL_FALSE:			n = "GL_FALSE";				break;
	case GL_TRUE:			n = "GL_TRUE";				break;
	default:				n = va("0x%X", normalized);	break;
	}

	fprintf(glwState.logFile, "glVertexAttribPointer( %u, %i, %s, %s, %i, %p )\n", index, size, t, n, stride, pointer);
	dllVertexAttribPointerARB(index, size, type, normalized, stride, pointer);
}

static GLvoid APIENTRY logEnableVertexAttribArrayARB (GLuint index){

	fprintf(glwState.logFile, "glEnableVertexAttribArray( %u )\n", index);
	dllEnableVertexAttribArrayARB(index);
}

static GLvoid APIENTRY logDisableVertexAttribArrayARB (GLuint index){

	fprintf(glwState.logFile, "glDisableVertexAttribArray( %u )\n", index);
	dllDisableVertexAttribArrayARB(index);
}

static GLvoid APIENTRY logBindProgramARB (GLenum target, GLuint program){

	char	*t;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:		t = "GL_VERTEX_PROGRAM";	break;
	case GL_FRAGMENT_PROGRAM_ARB:	t = "GL_FRAGMENT_PROGRAM";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glBindProgram( %s, %u )\n", t, program);
	dllBindProgramARB(target, program);
}

static GLvoid APIENTRY logDeleteProgramsARB (GLsizei n, const GLuint *programs){

	fprintf(glwState.logFile, "glDeletePrograms\n");
	dllDeleteProgramsARB(n, programs);
}

static GLvoid APIENTRY logGenProgramsARB (GLsizei n, GLuint *programs){

	fprintf(glwState.logFile, "glGenPrograms\n");
	dllGenProgramsARB(n, programs);
}

static GLvoid APIENTRY logProgramStringARB (GLenum target, GLenum format, GLsizei len, const GLvoid *string){

	char	*t, *f;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:			t = "GL_VERTEX_PROGRAM";		break;
	case GL_FRAGMENT_PROGRAM_ARB:		t = "GL_FRAGMENT_PROGRAM";		break;
	default:							t = va("0x%X", target);			break;
	}

	switch (format){
	case GL_PROGRAM_FORMAT_ASCII_ARB:	f = "GL_PROGRAM_FORMAT_ASCII";	break;
	default:							f = va("0x%X", format);			break;
	}

	fprintf(glwState.logFile, "glProgramString( %s, %s, %i, %p )\n", t, f, len, string);
	dllProgramStringARB(target, format, len, string);
}

static GLvoid APIENTRY logProgramEnvParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	char	*t;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:		t = "GL_VERTEX_PROGRAM";	break;
	case GL_FRAGMENT_PROGRAM_ARB:	t = "GL_FRAGMENT_PROGRAM";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glProgramEnvParameter4f( %s, %u, %g, %g, %g, %g )\n", t, index, x, y, z, w);
	dllProgramEnvParameter4fARB(target, index, x, y, z, w);
}

static GLvoid APIENTRY logProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat *params){

	char	*t;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:		t = "GL_VERTEX_PROGRAM";	break;
	case GL_FRAGMENT_PROGRAM_ARB:	t = "GL_FRAGMENT_PROGRAM";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glProgramEnvParameter4fv( %s, %u, %p )\n", t, index, params);
	dllProgramEnvParameter4fvARB(target, index, params);
}

static GLvoid APIENTRY logProgramLocalParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	char	*t;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:		t = "GL_VERTEX_PROGRAM";	break;
	case GL_FRAGMENT_PROGRAM_ARB:	t = "GL_FRAGMENT_PROGRAM";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glProgramLocalParameter4f( %s, %u, %g, %g, %g, %g )\n", t, index, x, y, z, w);
	dllProgramLocalParameter4fARB(target, index, x, y, z, w);
}

static GLvoid APIENTRY logProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat *params){

	char	*t;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:		t = "GL_VERTEX_PROGRAM";	break;
	case GL_FRAGMENT_PROGRAM_ARB:	t = "GL_FRAGMENT_PROGRAM";	break;
	default:						t = va("0x%X", target);		break;
	}

	fprintf(glwState.logFile, "glProgramLocalParameter4fv( %s, %u, %p )\n", t, index, params);
	dllProgramLocalParameter4fvARB(target, index, params);
}

static GLvoid APIENTRY logGetProgramivARB (GLenum target, GLenum pname, GLint *params){

	fprintf(glwState.logFile, "glGetProgramiv\n");
	dllGetProgramivARB(target, pname, params);
}

static GLvoid APIENTRY logDrawRangeElementsEXT (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices){

	char	*m, *t;

	switch (mode){
	case GL_POINTS:			m = "GL_POINTS";			break;
	case GL_LINES:			m = "GL_LINES";				break;
	case GL_LINE_STRIP:		m = "GL_LINE_STRIP";		break;
	case GL_LINE_LOOP:		m = "GL_LINE_LOOP";			break;
	case GL_TRIANGLES:		m = "GL_TRIANGLES";			break;
	case GL_TRIANGLE_STRIP:	m = "GL_TRIANGLE_STRIP";	break;
	case GL_TRIANGLE_FAN:	m = "GL_TRIANGLE_FAN";		break;
	case GL_QUADS:			m = "GL_QUADS";				break;
	case GL_QUAD_STRIP:		m = "GL_QUAD_STRIP";		break;
	case GL_POLYGON:		m = "GL_POLYGON";			break;
	default:				m = va("0x%X", mode);		break;
	}

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";				break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";		break;
	case GL_SHORT:			t = "GL_SHORT";				break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";	break;
	case GL_INT:			t = "GL_INT";				break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";		break;
	case GL_FLOAT:			t = "GL_FLOAT";				break;
	case GL_DOUBLE:			t = "GL_DOUBLE";			break;
	default:				t = va("0x%X", type);		break;
	}

	fprintf(glwState.logFile, "glDrawRangeElements( %s, %u, %u, %i, %s, %p )\n", m, start, end, count, t, indices);
	dllDrawRangeElementsEXT(mode, start, end, count, type, indices);
}

static GLvoid APIENTRY logActiveStencilFaceEXT (GLenum face){

	char	*f;

	switch (face){
	case GL_FRONT:	f = "GL_FRONT";			break;
	case GL_BACK:	f = "GL_BACK";			break;
	default:		f = va("0x%X", face);	break;
	}

	fprintf(glwState.logFile, "glActiveStencilFace( %s )\n", f);
	dllActiveStencilFaceEXT(face);
}

static GLvoid APIENTRY logDepthBoundsEXT (GLclampd zmin, GLclampd zmax){

	fprintf(glwState.logFile, "glDepthBounds( %g, %g )\n", zmin, zmax);
	dllDepthBoundsEXT(zmin, zmax);
}

static BOOL WINAPI logSwapIntervalEXT (int interval){

	fprintf(glwState.logFile, "wglSwapInterval\n");
	return dllSwapIntervalEXT(interval);
}

static GLvoid APIENTRY logCombinerParameterfvNV (GLenum pname, const GLfloat *params){

	char	*n;

	switch (pname){
	case GL_CONSTANT_COLOR0_NV:			n = "GL_CONSTANT_COLOR0";		break;
	case GL_CONSTANT_COLOR1_NV:			n = "GL_CONSTANT_COLOR1";		break;
	case GL_NUM_GENERAL_COMBINERS_NV:	n = "GL_NUM_GENERAL_COMBINERS";	break;
	case GL_COLOR_SUM_CLAMP_NV:			n = "GL_COLOR_SUM_CLAMP";		break;
	default:							n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glCombinerParameterfv( %s, %p )\n", n, params);
	dllCombinerParameterfvNV(pname, params);
}

static GLvoid APIENTRY logCombinerParameterfNV (GLenum pname, GLfloat param){

	char	*n;

	switch (pname){
	case GL_NUM_GENERAL_COMBINERS_NV:	n = "GL_NUM_GENERAL_COMBINERS";	break;
	case GL_COLOR_SUM_CLAMP_NV:			n = "GL_COLOR_SUM_CLAMP";		break;
	default:							n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glCombinerParameterf( %s, %g )\n", n, param);
	dllCombinerParameterfNV(pname, param);
}
	
static GLvoid APIENTRY logCombinerParameterivNV (GLenum pname, const GLint *params){

	char	*n;

	switch (pname){
	case GL_CONSTANT_COLOR0_NV:			n = "GL_CONSTANT_COLOR0";		break;
	case GL_CONSTANT_COLOR1_NV:			n = "GL_CONSTANT_COLOR1";		break;
	case GL_NUM_GENERAL_COMBINERS_NV:	n = "GL_NUM_GENERAL_COMBINERS";	break;
	case GL_COLOR_SUM_CLAMP_NV:			n = "GL_COLOR_SUM_CLAMP";		break;
	default:							n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glCombinerParameteriv( %s, %p )\n", n, params);
	dllCombinerParameterivNV(pname, params);
}

static GLvoid APIENTRY logCombinerParameteriNV (GLenum pname, GLint param){

	char	*n;

	switch (pname){
	case GL_NUM_GENERAL_COMBINERS_NV:	n = "GL_NUM_GENERAL_COMBINERS";	break;
	case GL_COLOR_SUM_CLAMP_NV:			n = "GL_COLOR_SUM_CLAMP";		break;
	default:							n = va("0x%X", pname);			break;
	}

	fprintf(glwState.logFile, "glCombinerParameteri( %s, %i )\n", n, param);
	dllCombinerParameteriNV(pname, param);
}

static GLvoid APIENTRY logCombinerInputNV (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage){

	char	*s, *p, *v, *i, *m, *cu;

	switch (stage){
	case GL_COMBINER0_NV:			s = "GL_COMBINER0";					break;
	case GL_COMBINER1_NV:			s = "GL_COMBINER1";					break;
	case GL_COMBINER2_NV:			s = "GL_COMBINER2";					break;
	case GL_COMBINER3_NV:			s = "GL_COMBINER3";					break;
	case GL_COMBINER4_NV:			s = "GL_COMBINER4";					break;
	case GL_COMBINER5_NV:			s = "GL_COMBINER5";					break;
	case GL_COMBINER6_NV:			s = "GL_COMBINER6";					break;
	case GL_COMBINER7_NV:			s = "GL_COMBINER7";					break;
	default:						s = va("0x%X", stage);				break;
	}

	switch (portion){
	case GL_RGB:					p = "GL_RGB";						break;
	case GL_ALPHA:					p = "GL_ALPHA";						break;
	default:						p = va("0x%X", portion);			break;
	}

	switch (variable){
	case GL_VARIABLE_A_NV:			v = "GL_VARIABLE_A";				break;
	case GL_VARIABLE_B_NV:			v = "GL_VARIABLE_B";				break;
	case GL_VARIABLE_C_NV:			v = "GL_VARIABLE_C";				break;
	case GL_VARIABLE_D_NV:			v = "GL_VARIABLE_D";				break;
	default:						v = va("0x%X", variable);			break;
	}

	switch (input){
	case GL_ZERO:					i = "GL_ZERO";						break;
	case GL_CONSTANT_COLOR0_NV:		i = "GL_CONSTANT_COLOR0";			break;
	case GL_CONSTANT_COLOR1_NV:		i = "GL_CONSTANT_COLOR1";			break;
	case GL_FOG:					i = "GL_FOG";						break;
	case GL_PRIMARY_COLOR_NV:		i = "GL_PRIMARY_COLOR";				break;
	case GL_SECONDARY_COLOR_NV:		i = "GL_SECONDARY_COLOR";			break;
	case GL_SPARE0_NV:				i = "GL_SPARE0";					break;
	case GL_SPARE1_NV:				i = "GL_SPARE1";					break;
	default:						i = va("0x%X", input);				break;
	}

	if (input >= GL_TEXTURE0_ARB && input <= GL_TEXTURE31_ARB)
		i = va("GL_TEXTURE%i", input - GL_TEXTURE0_ARB);
	else
		i = va("0x%X", input);

	switch (mapping){
	case GL_UNSIGNED_IDENTITY_NV:	m = "GL_UNSIGNED_IDENTITY";			break;
	case GL_UNSIGNED_INVERT_NV:		m = "GL_UNSIGNED_INVERT";			break;
	case GL_EXPAND_NORMAL_NV:		m = "GL_EXPAND_NORMAL";				break;
	case GL_EXPAND_NEGATE_NV:		m = "GL_EXPAND_NEGATE";				break;
	case GL_HALF_BIAS_NORMAL_NV:	m = "GL_HALF_BIAS_NORMAL";			break;
	case GL_HALF_BIAS_NEGATE_NV:	m = "GL_HALF_BIAS_NEGATE";			break;
	case GL_SIGNED_IDENTITY_NV:		m = "GL_SIGNED_IDENTITY";			break;
	case GL_SIGNED_NEGATE_NV:		m = "GL_SIGNED_NEGATE";				break;
	default:						m = va("0x%X", mapping);			break;
	}

	switch (componentUsage){
	case GL_RGB:					cu = "GL_RGB";						break;
	case GL_ALPHA:					cu = "GL_ALPHA";					break;
	case GL_BLUE:					cu = "GL_BLUE";						break;
	default:						cu = va("0x%X", componentUsage);	break;
	}

	fprintf(glwState.logFile, "glCombinerInput( %s, %s, %s, %s, %s, %s )\n", s, p, v, i, m, cu);
	dllCombinerInputNV(stage, portion, variable, input, mapping, componentUsage);
}

static GLvoid APIENTRY logCombinerOutputNV (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum){

	char	*s, *p, *abo, *cdo, *so, *sc, *bi, *abdp, *cddp, *ms;

	switch (stage){
	case GL_COMBINER0_NV:					s = "GL_COMBINER0";						break;
	case GL_COMBINER1_NV:					s = "GL_COMBINER1";						break;
	case GL_COMBINER2_NV:					s = "GL_COMBINER2";						break;
	case GL_COMBINER3_NV:					s = "GL_COMBINER3";						break;
	case GL_COMBINER4_NV:					s = "GL_COMBINER4";						break;
	case GL_COMBINER5_NV:					s = "GL_COMBINER5";						break;
	case GL_COMBINER6_NV:					s = "GL_COMBINER6";						break;
	case GL_COMBINER7_NV:					s = "GL_COMBINER7";						break;
	default:								s = va("0x%X", stage);					break;
	}

	switch (portion){
	case GL_RGB:							p = "GL_RGB";							break;
	case GL_ALPHA:							p = "GL_ALPHA";							break;
	default:								p = va("0x%X", portion);				break;
	}

	switch (abOutput){
	case GL_DISCARD_NV:						abo = "GL_DISCARD";						break;
	case GL_PRIMARY_COLOR_NV:				abo = "GL_PRIMARY_COLOR";				break;
	case GL_SECONDARY_COLOR_NV:				abo = "GL_SECONDARY_COLOR";				break;
	case GL_SPARE0_NV:						abo = "GL_SPARE0";						break;
	case GL_SPARE1_NV:						abo = "GL_SPARE1";						break;
	default:								abo = va("0x%X", abOutput);				break;
	}

	if (abOutput >= GL_TEXTURE0_ARB && abOutput <= GL_TEXTURE31_ARB)
		abo = va("GL_TEXTURE%i", abOutput - GL_TEXTURE0_ARB);
	else
		abo = va("0x%X", abOutput);

	switch (cdOutput){
	case GL_DISCARD_NV:						cdo = "GL_DISCARD";						break;
	case GL_PRIMARY_COLOR_NV:				cdo = "GL_PRIMARY_COLOR";				break;
	case GL_SECONDARY_COLOR_NV:				cdo = "GL_SECONDARY_COLOR";				break;
	case GL_SPARE0_NV:						cdo = "GL_SPARE0";						break;
	case GL_SPARE1_NV:						cdo = "GL_SPARE1";						break;
	default:								cdo = va("0x%X", cdOutput);				break;
	}

	if (cdOutput >= GL_TEXTURE0_ARB && cdOutput <= GL_TEXTURE31_ARB)
		cdo = va("GL_TEXTURE%i", cdOutput - GL_TEXTURE0_ARB);
	else
		cdo = va("0x%X", cdOutput);

	switch (sumOutput){
	case GL_DISCARD_NV:						so = "GL_DISCARD";						break;
	case GL_PRIMARY_COLOR_NV:				so = "GL_PRIMARY_COLOR";				break;
	case GL_SECONDARY_COLOR_NV:				so = "GL_SECONDARY_COLOR";				break;
	case GL_SPARE0_NV:						so = "GL_SPARE0";						break;
	case GL_SPARE1_NV:						so = "GL_SPARE1";						break;
	default:								so = va("0x%X", sumOutput);				break;
	}

	if (sumOutput >= GL_TEXTURE0_ARB && sumOutput <= GL_TEXTURE31_ARB)
		so = va("GL_TEXTURE%i", sumOutput - GL_TEXTURE0_ARB);
	else
		so = va("0x%X", sumOutput);

	switch (scale){
	case GL_NONE:							sc = "GL_NONE";							break;
	case GL_SCALE_BY_TWO_NV:				sc = "GL_SCALE_BY_TWO";					break;
	case GL_SCALE_BY_FOUR_NV:				sc = "GL_SCALE_BY_FOUR";				break;
	case GL_SCALE_BY_ONE_HALF_NV:			sc = "GL_SCALE_BY_ONE_HALF";			break;
	default:								sc = va("0x%X", scale);					break;
	}

	switch (bias){
	case GL_NONE:							bi = "GL_NONE";							break;
	case GL_BIAS_BY_NEGATIVE_ONE_HALF_NV:	bi = "GL_BIAS_BY_NEGATIVE_ONE_HALF";	break;
	default:								bi = va("0x%X", bias);					break;
	}

	switch (abDotProduct){
	case GL_FALSE:							abdp = "GL_FALSE";						break;
	case GL_TRUE:							abdp = "GL_TRUE";						break;
	default:								abdp = va("0x%X", abDotProduct);		break;
	}

	switch (cdDotProduct){
	case GL_FALSE:							cddp = "GL_FALSE";						break;
	case GL_TRUE:							cddp = "GL_TRUE";						break;
	default:								cddp = va("0x%X", cdDotProduct);		break;
	}

	switch (muxSum){
	case GL_FALSE:							ms = "GL_FALSE";						break;
	case GL_TRUE:							ms = "GL_TRUE";							break;
	default:								ms = va("0x%X", muxSum);				break;
	}

	fprintf(glwState.logFile, "glCombinerOutput( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s )\n", s, p, abo, cdo, so, sc, bi, abdp, cddp, ms);
	dllCombinerOutputNV(stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);
}

static GLvoid APIENTRY logFinalCombinerInputNV (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage){

	char	*v, *i, *m, *cu;

	switch (variable){
	case GL_VARIABLE_A_NV:					v = "GL_VARIABLE_A";					break;
	case GL_VARIABLE_B_NV:					v = "GL_VARIABLE_B";					break;
	case GL_VARIABLE_C_NV:					v = "GL_VARIABLE_C";					break;
	case GL_VARIABLE_D_NV:					v = "GL_VARIABLE_D";					break;
	case GL_VARIABLE_E_NV:					v = "GL_VARIABLE_E";					break;
	case GL_VARIABLE_F_NV:					v = "GL_VARIABLE_F";					break;
	case GL_VARIABLE_G_NV:					v = "GL_VARIABLE_G";					break;
	default:								v = va("0x%X", variable);				break;
	}

	switch (input){
	case GL_ZERO:							i = "GL_ZERO";							break;
	case GL_CONSTANT_COLOR0_NV:				i = "GL_CONSTANT_COLOR0";				break;
	case GL_CONSTANT_COLOR1_NV:				i = "GL_CONSTANT_COLOR1";				break;
	case GL_FOG:							i = "GL_FOG";							break;
	case GL_PRIMARY_COLOR_NV:				i = "GL_PRIMARY_COLOR";					break;
	case GL_SECONDARY_COLOR_NV:				i = "GL_SECONDARY_COLOR";				break;
	case GL_SPARE0_NV:						i = "GL_SPARE0";						break;
	case GL_SPARE1_NV:						i = "GL_SPARE1";						break;
	case GL_E_TIMES_F_NV:					i = "GL_E_TIMES_F";						break;
	case GL_SPARE0_PLUS_SECONDARY_COLOR_NV:	i = "GL_SPARE0_PLUS_SECONDARY_COLOR";	break;
	default:								i = va("0x%X", input);					break;
	}

	if (input >= GL_TEXTURE0_ARB && input <= GL_TEXTURE31_ARB)
		i = va("GL_TEXTURE%i", input - GL_TEXTURE0_ARB);
	else
		i = va("0x%X", input);

	switch (mapping){
	case GL_UNSIGNED_IDENTITY_NV:			m = "GL_UNSIGNED_IDENTITY";				break;
	case GL_UNSIGNED_INVERT_NV:				m = "GL_UNSIGNED_INVERT";				break;
	default:								m = va("0x%X", mapping);				break;
	}

	switch (componentUsage){
	case GL_RGB:							cu = "GL_RGB";							break;
	case GL_ALPHA:							cu = "GL_ALPHA";						break;
	case GL_BLUE:							cu = "GL_BLUE";							break;
	default:								cu = va("0x%X", componentUsage);		break;
	}

	fprintf(glwState.logFile, "glFinalCombinerInput( %s, %s, %s, %s )\n", v, i, m, cu);
	dllFinalCombinerInputNV(variable, input, mapping, componentUsage);
}

static GLuint APIENTRY logGenFragmentShadersATI (GLuint range){

	fprintf(glwState.logFile, "glGenFragmentShaders\n");
	return dllGenFragmentShadersATI(range);
}

static GLvoid APIENTRY logBindFragmentShaderATI (GLuint id){

	fprintf(glwState.logFile, "glBindFragmentShader( %u )\n", id);
	dllBindFragmentShaderATI(id);
}

static GLvoid APIENTRY logDeleteFragmentShaderATI (GLuint id){

	fprintf(glwState.logFile, "glDeleteFragmentShader\n");
	dllDeleteFragmentShaderATI(id);
}

static GLvoid APIENTRY logBeginFragmentShaderATI (GLvoid){

	fprintf(glwState.logFile, "glBeginFragmentShader\n");
	dllBeginFragmentShaderATI();
}

static GLvoid APIENTRY logEndFragmentShaderATI (GLvoid){

	fprintf(glwState.logFile, "glEndFragmentShader\n");
	dllEndFragmentShaderATI();
}

static GLvoid APIENTRY logPassTexCoordATI (GLuint dst, GLuint coord, GLenum swizzle){

	char	*d, *c, *s;

	switch (dst){
	case GL_REG_0_ATI:			d = "GL_REG_0";				break;
	case GL_REG_1_ATI:			d = "GL_REG_1";				break;
	case GL_REG_2_ATI:			d = "GL_REG_2";				break;
	case GL_REG_3_ATI:			d = "GL_REG_3";				break;
	case GL_REG_4_ATI:			d = "GL_REG_4";				break;
	case GL_REG_5_ATI:			d = "GL_REG_5";				break;
	default:					d = va("0x%X", dst);		break;
	}

	switch (coord){
	case GL_REG_0_ATI:			c = "GL_REG_0";				break;
	case GL_REG_1_ATI:			c = "GL_REG_1";				break;
	case GL_REG_2_ATI:			c = "GL_REG_2";				break;
	case GL_REG_3_ATI:			c = "GL_REG_3";				break;
	case GL_REG_4_ATI:			c = "GL_REG_4";				break;
	case GL_REG_5_ATI:			c = "GL_REG_5";				break;
	default:					c = va("0x%X", coord);		break;
	}

	if (coord >= GL_TEXTURE0_ARB && coord <= GL_TEXTURE31_ARB)
		c = va("GL_TEXTURE%i", coord - GL_TEXTURE0_ARB);
	else
		c = va("0x%X", coord);

	switch (swizzle){
	case GL_SWIZZLE_STR_ATI:	s = "GL_SWIZZLE_STR";		break;
	case GL_SWIZZLE_STQ_ATI:	s = "GL_SWIZZLE_STQ";		break;
	case GL_SWIZZLE_STR_DR_ATI:	s = "GL_SWIZZLE_STR_DR";	break;
	case GL_SWIZZLE_STQ_DQ_ATI:	s = "GL_SWIZZLE_STQ_DQ";	break;
	default:					s = va("0x%X", swizzle);	break;
	}

	fprintf(glwState.logFile, "glPassTexCoord( %s, %s, %s )\n", d, c, s);
	dllPassTexCoordATI(dst, coord, swizzle);
}

static GLvoid APIENTRY logSampleMapATI (GLuint dst, GLuint interp, GLenum swizzle){

	char	*d, *i, *s;

	switch (dst){
	case GL_REG_0_ATI:			d = "GL_REG_0";				break;
	case GL_REG_1_ATI:			d = "GL_REG_1";				break;
	case GL_REG_2_ATI:			d = "GL_REG_2";				break;
	case GL_REG_3_ATI:			d = "GL_REG_3";				break;
	case GL_REG_4_ATI:			d = "GL_REG_4";				break;
	case GL_REG_5_ATI:			d = "GL_REG_5";				break;
	default:					d = va("0x%X", dst);		break;
	}

	switch (interp){
	case GL_REG_0_ATI:			i = "GL_REG_0";				break;
	case GL_REG_1_ATI:			i = "GL_REG_1";				break;
	case GL_REG_2_ATI:			i = "GL_REG_2";				break;
	case GL_REG_3_ATI:			i = "GL_REG_3";				break;
	case GL_REG_4_ATI:			i = "GL_REG_4";				break;
	case GL_REG_5_ATI:			i = "GL_REG_5";				break;
	default:					i = va("0x%X", interp);		break;
	}

	if (interp >= GL_TEXTURE0_ARB && interp <= GL_TEXTURE31_ARB)
		i = va("GL_TEXTURE%i", interp - GL_TEXTURE0_ARB);
	else
		i = va("0x%X", interp);

	switch (swizzle){
	case GL_SWIZZLE_STR_ATI:	s = "GL_SWIZZLE_STR";		break;
	case GL_SWIZZLE_STQ_ATI:	s = "GL_SWIZZLE_STQ";		break;
	case GL_SWIZZLE_STR_DR_ATI:	s = "GL_SWIZZLE_STR_DR";	break;
	case GL_SWIZZLE_STQ_DQ_ATI:	s = "GL_SWIZZLE_STQ_DQ";	break;
	default:					s = va("0x%X", swizzle);	break;
	}

	fprintf(glwState.logFile, "glSampleMap( %s, %s, %s )\n", d, i, s);
	dllSampleMapATI(dst, interp, swizzle);
}

static GLvoid APIENTRY logColorFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod){

	char	*o, *d, *dma, *dmo, *a1, *a1r, *a1m;

	switch (op){
	case GL_MOV_ATI:					o = "GL_MOV";						break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMask){
	case GL_NONE:						dma = "GL_NONE";					break;
	case GL_RED_BIT_ATI:				dma = "GL_RED_BIT";					break;
	case GL_GREEN_BIT_ATI:				dma = "GL_GREEN_BIT";				break;
	case GL_BLUE_BIT_ATI:				dma = "GL_BLUE_BIT";				break;
	default:							dma = va("0x%X", dstMask);			break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	fprintf(glwState.logFile, "glColorFragmentOp1( %s, %s, %s, %s, %s, %s, %s )\n", o, d, dma, dmo, a1, a1r, a1m);
	dllColorFragmentOp1ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);
}

static GLvoid APIENTRY logColorFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod){

	char	*o, *d, *dma, *dmo, *a1, *a1r, *a1m, *a2, *a2r, *a2m;

	switch (op){
	case GL_ADD_ATI:					o = "GL_ADD";						break;
	case GL_MUL_ATI:					o = "GL_MUL";						break;
	case GL_SUB_ATI:					o = "GL_SUB";						break;
	case GL_DOT3_ATI:					o = "GL_DOT3";						break;
	case GL_DOT4_ATI:					o = "GL_DOT4";						break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMask){
	case GL_NONE:						dma = "GL_NONE";					break;
	case GL_RED_BIT_ATI:				dma = "GL_RED_BIT";					break;
	case GL_GREEN_BIT_ATI:				dma = "GL_GREEN_BIT";				break;
	case GL_BLUE_BIT_ATI:				dma = "GL_BLUE_BIT";				break;
	default:							dma = va("0x%X", dstMask);			break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	switch (arg2){
	case GL_REG_0_ATI:					a2 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a2 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a2 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a2 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a2 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a2 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a2 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a2 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a2 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a2 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a2 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a2 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a2 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a2 = "GL_CON_7";					break;
	case GL_ZERO:						a2 = "GL_ZERO";						break;
	case GL_ONE:						a2 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a2 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a2 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a2 = va("0x%X", arg2);				break;
	}

	switch (arg2Rep){
	case GL_NONE:						a2r = "GL_NONE";					break;
	case GL_RED:						a2r = "GL_RED";						break;
	case GL_GREEN:						a2r = "GL_GREEN";					break;
	case GL_BLUE:						a2r = "GL_BLUE";					break;
	case GL_ALPHA:						a2r = "GL_ALPHA";					break;
	default:							a2r = va("0x%X", arg2Rep);			break;
	}

	switch (arg2Mod){
	case GL_2X_BIT_ATI:					a2m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a2m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a2m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a2m = "GL_BIAS_BIT";				break;
	default:							a2m = va("0x%X", arg2Mod);			break;
	}

	fprintf(glwState.logFile, "glColorFragmentOp2( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s )\n", o, d, dma, dmo, a1, a1r, a1m, a2, a2r, a2m);
	dllColorFragmentOp2ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
}

static GLvoid APIENTRY logColorFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod){

	char	*o, *d, *dma, *dmo, *a1, *a1r, *a1m, *a2, *a2r, *a2m, *a3, *a3r, *a3m;

	switch (op){
	case GL_MAD_ATI:					o = "GL_MAD";						break;
	case GL_LERP_ATI:					o = "GL_LERP";						break;
	case GL_CND_ATI:					o = "GL_CND";						break;
	case GL_CND0_ATI:					o = "GL_CND0";						break;
	case GL_DOT2_ADD_ATI:				o = "GL_DOT2_ADD";					break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMask){
	case GL_NONE:						dma = "GL_NONE";					break;
	case GL_RED_BIT_ATI:				dma = "GL_RED_BIT";					break;
	case GL_GREEN_BIT_ATI:				dma = "GL_GREEN_BIT";				break;
	case GL_BLUE_BIT_ATI:				dma = "GL_BLUE_BIT";				break;
	default:							dma = va("0x%X", dstMask);			break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	switch (arg2){
	case GL_REG_0_ATI:					a2 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a2 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a2 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a2 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a2 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a2 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a2 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a2 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a2 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a2 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a2 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a2 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a2 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a2 = "GL_CON_7";					break;
	case GL_ZERO:						a2 = "GL_ZERO";						break;
	case GL_ONE:						a2 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a2 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a2 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a2 = va("0x%X", arg2);				break;
	}

	switch (arg2Rep){
	case GL_NONE:						a2r = "GL_NONE";					break;
	case GL_RED:						a2r = "GL_RED";						break;
	case GL_GREEN:						a2r = "GL_GREEN";					break;
	case GL_BLUE:						a2r = "GL_BLUE";					break;
	case GL_ALPHA:						a2r = "GL_ALPHA";					break;
	default:							a2r = va("0x%X", arg2Rep);			break;
	}

	switch (arg2Mod){
	case GL_2X_BIT_ATI:					a2m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a2m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a2m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a2m = "GL_BIAS_BIT";				break;
	default:							a2m = va("0x%X", arg2Mod);			break;
	}

	switch (arg3){
	case GL_REG_0_ATI:					a3 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a3 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a3 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a3 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a3 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a3 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a3 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a3 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a3 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a3 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a3 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a3 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a3 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a3 = "GL_CON_7";					break;
	case GL_ZERO:						a3 = "GL_ZERO";						break;
	case GL_ONE:						a3 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a3 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a3 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a3 = va("0x%X", arg3);				break;
	}

	switch (arg3Rep){
	case GL_NONE:						a3r = "GL_NONE";					break;
	case GL_RED:						a3r = "GL_RED";						break;
	case GL_GREEN:						a3r = "GL_GREEN";					break;
	case GL_BLUE:						a3r = "GL_BLUE";					break;
	case GL_ALPHA:						a3r = "GL_ALPHA";					break;
	default:							a3r = va("0x%X", arg3Rep);			break;
	}

	switch (arg3Mod){
	case GL_2X_BIT_ATI:					a3m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a3m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a3m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a3m = "GL_BIAS_BIT";				break;
	default:							a3m = va("0x%X", arg3Mod);			break;
	}

	fprintf(glwState.logFile, "glColorFragmentOp3( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s )\n", o, d, dma, dmo, a1, a1r, a1m, a2, a2r, a2m, a3, a3r, a3m);
	dllColorFragmentOp3ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
}

static GLvoid APIENTRY logAlphaFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod){

	char	*o, *d, *dmo, *a1, *a1r, *a1m;

	switch (op){
	case GL_MOV_ATI:					o = "GL_MOV";						break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	fprintf(glwState.logFile, "glAlphaFragmentOp1( %s, %s, %s, %s, %s, %s )\n", o, d, dmo, a1, a1r, a1m);
	dllAlphaFragmentOp1ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod);
}

static GLvoid APIENTRY logAlphaFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod){

	char	*o, *d, *dmo, *a1, *a1r, *a1m, *a2, *a2r, *a2m;

	switch (op){
	case GL_ADD_ATI:					o = "GL_ADD";						break;
	case GL_MUL_ATI:					o = "GL_MUL";						break;
	case GL_SUB_ATI:					o = "GL_SUB";						break;
	case GL_DOT3_ATI:					o = "GL_DOT3";						break;
	case GL_DOT4_ATI:					o = "GL_DOT4";						break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	switch (arg2){
	case GL_REG_0_ATI:					a2 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a2 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a2 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a2 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a2 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a2 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a2 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a2 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a2 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a2 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a2 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a2 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a2 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a2 = "GL_CON_7";					break;
	case GL_ZERO:						a2 = "GL_ZERO";						break;
	case GL_ONE:						a2 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a2 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a2 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a2 = va("0x%X", arg2);				break;
	}

	switch (arg2Rep){
	case GL_NONE:						a2r = "GL_NONE";					break;
	case GL_RED:						a2r = "GL_RED";						break;
	case GL_GREEN:						a2r = "GL_GREEN";					break;
	case GL_BLUE:						a2r = "GL_BLUE";					break;
	case GL_ALPHA:						a2r = "GL_ALPHA";					break;
	default:							a2r = va("0x%X", arg2Rep);			break;
	}

	switch (arg2Mod){
	case GL_2X_BIT_ATI:					a2m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a2m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a2m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a2m = "GL_BIAS_BIT";				break;
	default:							a2m = va("0x%X", arg2Mod);			break;
	}

	fprintf(glwState.logFile, "glAlphaFragmentOp2( %s, %s, %s, %s, %s, %s, %s, %s, %s )\n", o, d, dmo, a1, a1r, a1m, a2, a2r, a2m);
	dllAlphaFragmentOp2ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
}

static GLvoid APIENTRY logAlphaFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod){

	char	*o, *d, *dmo, *a1, *a1r, *a1m, *a2, *a2r, *a2m, *a3, *a3r, *a3m;

	switch (op){
	case GL_MAD_ATI:					o = "GL_MAD";						break;
	case GL_LERP_ATI:					o = "GL_LERP";						break;
	case GL_CND_ATI:					o = "GL_CND";						break;
	case GL_CND0_ATI:					o = "GL_CND0";						break;
	case GL_DOT2_ADD_ATI:				o = "GL_DOT2_ADD";					break;
	default:							o = va("0x%X", op);					break;
	}

	switch (dst){
	case GL_REG_0_ATI:					d = "GL_REG_0";						break;
	case GL_REG_1_ATI:					d = "GL_REG_1";						break;
	case GL_REG_2_ATI:					d = "GL_REG_2";						break;
	case GL_REG_3_ATI:					d = "GL_REG_3";						break;
	case GL_REG_4_ATI:					d = "GL_REG_4";						break;
	case GL_REG_5_ATI:					d = "GL_REG_5";						break;
	default:							d = va("0x%X", dst);				break;
	}

	switch (dstMod){
	case GL_NONE:						dmo = "GL_NONE";					break;
	case GL_2X_BIT_ATI:					dmo = "GL_2X_BIT";					break;
	case GL_4X_BIT_ATI:					dmo = "GL_4X_BIT";					break;
	case GL_8X_BIT_ATI:					dmo = "GL_8X_BIT";					break;
	case GL_HALF_BIT_ATI:				dmo = "GL_HALF_BIT";				break;
	case GL_QUARTER_BIT_ATI:			dmo = "GL_QUARTER_BIT";				break;
	case GL_EIGHTH_BIT_ATI:				dmo = "GL_EIGHTH_BIT";				break;
	case GL_SATURATE_BIT_ATI:			dmo = "GL_SATURATE_BIT";			break;
	default:							dmo = va("0x%X", dstMod);			break;
	}

	switch (arg1){
	case GL_REG_0_ATI:					a1 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a1 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a1 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a1 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a1 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a1 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a1 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a1 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a1 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a1 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a1 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a1 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a1 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a1 = "GL_CON_7";					break;
	case GL_ZERO:						a1 = "GL_ZERO";						break;
	case GL_ONE:						a1 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a1 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a1 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a1 = va("0x%X", arg1);				break;
	}

	switch (arg1Rep){
	case GL_NONE:						a1r = "GL_NONE";					break;
	case GL_RED:						a1r = "GL_RED";						break;
	case GL_GREEN:						a1r = "GL_GREEN";					break;
	case GL_BLUE:						a1r = "GL_BLUE";					break;
	case GL_ALPHA:						a1r = "GL_ALPHA";					break;
	default:							a1r = va("0x%X", arg1Rep);			break;
	}

	switch (arg1Mod){
	case GL_2X_BIT_ATI:					a1m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a1m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a1m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a1m = "GL_BIAS_BIT";				break;
	default:							a1m = va("0x%X", arg1Mod);			break;
	}

	switch (arg2){
	case GL_REG_0_ATI:					a2 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a2 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a2 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a2 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a2 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a2 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a2 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a2 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a2 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a2 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a2 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a2 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a2 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a2 = "GL_CON_7";					break;
	case GL_ZERO:						a2 = "GL_ZERO";						break;
	case GL_ONE:						a2 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a2 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a2 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a2 = va("0x%X", arg2);				break;
	}

	switch (arg2Rep){
	case GL_NONE:						a2r = "GL_NONE";					break;
	case GL_RED:						a2r = "GL_RED";						break;
	case GL_GREEN:						a2r = "GL_GREEN";					break;
	case GL_BLUE:						a2r = "GL_BLUE";					break;
	case GL_ALPHA:						a2r = "GL_ALPHA";					break;
	default:							a2r = va("0x%X", arg2Rep);			break;
	}

	switch (arg2Mod){
	case GL_2X_BIT_ATI:					a2m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a2m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a2m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a2m = "GL_BIAS_BIT";				break;
	default:							a2m = va("0x%X", arg2Mod);			break;
	}

	switch (arg3){
	case GL_REG_0_ATI:					a3 = "GL_REG_0";					break;
	case GL_REG_1_ATI:					a3 = "GL_REG_1";					break;
	case GL_REG_2_ATI:					a3 = "GL_REG_2";					break;
	case GL_REG_3_ATI:					a3 = "GL_REG_3";					break;
	case GL_REG_4_ATI:					a3 = "GL_REG_4";					break;
	case GL_REG_5_ATI:					a3 = "GL_REG_5";					break;
	case GL_CON_0_ATI:					a3 = "GL_CON_0";					break;
	case GL_CON_1_ATI:					a3 = "GL_CON_1";					break;
	case GL_CON_2_ATI:					a3 = "GL_CON_2";					break;
	case GL_CON_3_ATI:					a3 = "GL_CON_3";					break;
	case GL_CON_4_ATI:					a3 = "GL_CON_4";					break;
	case GL_CON_5_ATI:					a3 = "GL_CON_5";					break;
	case GL_CON_6_ATI:					a3 = "GL_CON_6";					break;
	case GL_CON_7_ATI:					a3 = "GL_CON_7";					break;
	case GL_ZERO:						a3 = "GL_ZERO";						break;
	case GL_ONE:						a3 = "GL_ONE";						break;
	case GL_PRIMARY_COLOR_ARB:			a3 = "GL_PRIMARY_COLOR";			break;
	case GL_SECONDARY_INTERPOLATOR_ATI:	a3 = "GL_SECONDARY_INTERPOLATOR";	break;
	default:							a3 = va("0x%X", arg3);				break;
	}

	switch (arg3Rep){
	case GL_NONE:						a3r = "GL_NONE";					break;
	case GL_RED:						a3r = "GL_RED";						break;
	case GL_GREEN:						a3r = "GL_GREEN";					break;
	case GL_BLUE:						a3r = "GL_BLUE";					break;
	case GL_ALPHA:						a3r = "GL_ALPHA";					break;
	default:							a3r = va("0x%X", arg3Rep);			break;
	}

	switch (arg3Mod){
	case GL_2X_BIT_ATI:					a3m = "GL_2X_BIT";					break;
	case GL_COMP_BIT_ATI:				a3m = "GL_COMP_BIT";				break;
	case GL_NEGATE_BIT_ATI:				a3m = "GL_NEGATE_BIT";				break;
	case GL_BIAS_BIT_ATI:				a3m = "GL_BIAS_BIT";				break;
	default:							a3m = va("0x%X", arg3Mod);			break;
	}

	fprintf(glwState.logFile, "glAlphaFragmentOp3( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s )\n", o, d, dmo, a1, a1r, a1m, a2, a2r, a2m, a3, a3r, a3m);
	dllAlphaFragmentOp3ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
}

static GLvoid APIENTRY logSetFragmentShaderConstantATI (GLuint dst, const GLfloat *value){

	char	*d;

	switch (dst){
	case GL_CON_0_ATI:	d = "GL_CON_0";			break;
	case GL_CON_1_ATI:	d = "GL_CON_1";			break;
	case GL_CON_2_ATI:	d = "GL_CON_2";			break;
	case GL_CON_3_ATI:	d = "GL_CON_3";			break;
	case GL_CON_4_ATI:	d = "GL_CON_4";			break;
	case GL_CON_5_ATI:	d = "GL_CON_5";			break;
	case GL_CON_6_ATI:	d = "GL_CON_6";			break;
	case GL_CON_7_ATI:	d = "GL_CON_7";			break;
	default:			d = va("0x%X", dst);	break;
	}

	fprintf(glwState.logFile, "glSetFragmentShaderConstant( %s, %p )\n", d, value);
	dllSetFragmentShaderConstantATI(dst, value);
}

static GLvoid APIENTRY logStencilOpSeparateATI (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass){

	char	*f, *sf, *dpf, *dpp;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";				break;
	case GL_BACK:			f = "GL_BACK";				break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";	break;
	default:				f = va("0x%X", face);		break;
	}

	switch (sfail){
	case GL_KEEP:			sf = "GL_KEEP";				break;
	case GL_ZERO:			sf = "GL_ZERO";				break;
	case GL_REPLACE:		sf = "GL_REPLACE";			break;
	case GL_INCR:			sf = "GL_INCR";				break;
	case GL_INCR_WRAP_EXT:	sf = "GL_INCR_WRAP";		break;
	case GL_DECR:			sf = "GL_DECR";				break;
	case GL_DECR_WRAP_EXT:	sf = "GL_DECR_WRAP";		break;
	case GL_INVERT:			sf = "GL_INVERT";			break;
	default:				sf = va("0x%X", sfail);		break;
	}

	switch (dpfail){
	case GL_KEEP:			dpf = "GL_KEEP";			break;
	case GL_ZERO:			dpf = "GL_ZERO";			break;
	case GL_REPLACE:		dpf = "GL_REPLACE";			break;
	case GL_INCR:			dpf = "GL_INCR";			break;
	case GL_INCR_WRAP_EXT:	dpf = "GL_INCR_WRAP";		break;
	case GL_DECR:			dpf = "GL_DECR";			break;
	case GL_DECR_WRAP_EXT:	dpf = "GL_DECR_WRAP";		break;
	case GL_INVERT:			dpf = "GL_INVERT";			break;
	default:				dpf = va("0x%X", dpfail);	break;
	}

	switch (dppass){
	case GL_KEEP:			dpp = "GL_KEEP";			break;
	case GL_ZERO:			dpp = "GL_ZERO";			break;
	case GL_REPLACE:		dpp = "GL_REPLACE";			break;
	case GL_INCR:			dpp = "GL_INCR";			break;
	case GL_INCR_WRAP_EXT:	dpp = "GL_INCR_WRAP";		break;
	case GL_DECR:			dpp = "GL_DECR";			break;
	case GL_DECR_WRAP_EXT:	dpp = "GL_DECR_WRAP";		break;
	case GL_INVERT:			dpp = "GL_INVERT";			break;
	default:				dpp = va("0x%X", dppass);	break;
	}

	fprintf(glwState.logFile, "glStencilOpSeparate( %s, %s, %s, %s )\n", f, sf, dpf, dpp);
	dllStencilOpSeparateATI(face, sfail, dpfail, dppass);
}

static GLvoid APIENTRY logStencilFuncSeparateATI (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask){

	char	*ff, *bf;

	switch (frontfunc){
	case GL_NEVER:		ff = "GL_NEVER";			break;
	case GL_LESS:		ff = "GL_LESS";				break;
	case GL_LEQUAL:		ff = "GL_LEQUAL";			break;
	case GL_GREATER:	ff = "GL_GREATER";			break;
	case GL_GEQUAL:		ff = "GL_GEQUAL";			break;
	case GL_EQUAL:		ff = "GL_EQUAL";			break;
	case GL_NOTEQUAL:	ff = "GL_NOTEQUAL";			break;
	case GL_ALWAYS:		ff = "GL_ALWAYS";			break;
	default:			ff = va("0x%X", frontfunc);	break;
	}

	switch (backfunc){
	case GL_NEVER:		bf = "GL_NEVER";			break;
	case GL_LESS:		bf = "GL_LESS";				break;
	case GL_LEQUAL:		bf = "GL_LEQUAL";			break;
	case GL_GREATER:	bf = "GL_GREATER";			break;
	case GL_GEQUAL:		bf = "GL_GEQUAL";			break;
	case GL_EQUAL:		bf = "GL_EQUAL";			break;
	case GL_NOTEQUAL:	bf = "GL_NOTEQUAL";			break;
	case GL_ALWAYS:		bf = "GL_ALWAYS";			break;
	default:			bf = va("0x%X", backfunc);	break;
	}

	fprintf(glwState.logFile, "glStencilFuncSeparate( %s, %s, %i, %u )\n", ff, bf, ref, mask);
	dllStencilFuncSeparateATI(frontfunc, backfunc, ref, mask);
}


// =====================================================================

static qboolean	qglExtensionPointers;


/*
 =================
 QGL_GetProcAddress
 =================
*/
static void *QGL_GetProcAddress (const char *funcName){

	void	*funcAddress;

	funcAddress = GetProcAddress(glwState.hInstOpenGL, funcName);
	if (!funcAddress){
		FreeLibrary(glwState.hInstOpenGL);
		glwState.hInstOpenGL = NULL;

		Com_Error(ERR_FATAL, "QGL_GetProcAddress: GetProcAddress() failed for '%s'", funcName);
	}

	return funcAddress;
}

/*
 =================
 QGL_Shutdown

 Unloads the specified DLL then nulls out all the proc pointers
 =================
*/
void QGL_Shutdown (void){

	Com_Printf("...shutting down QGL\n");

	qglExtensionPointers = false;

	if (glwState.hInstOpenGL){
		Com_Printf("...unloading OpenGL DLL\n");

		FreeLibrary(glwState.hInstOpenGL);
		glwState.hInstOpenGL = NULL;
	}

	qwglChoosePixelFormat       = NULL;
	qwglDescribePixelFormat     = NULL;
	qwglGetPixelFormat          = NULL;
	qwglSetPixelFormat          = NULL;
	qwglSwapBuffers             = NULL;

	qwglCopyContext             = NULL;
	qwglCreateContext           = NULL;
	qwglCreateLayerContext      = NULL;
	qwglDeleteContext           = NULL;
	qwglDescribeLayerPlane      = NULL;
	qwglGetCurrentContext       = NULL;
	qwglGetCurrentDC            = NULL;
	qwglGetLayerPaletteEntries  = NULL;
	qwglGetProcAddress          = NULL;
	qwglMakeCurrent             = NULL;
	qwglRealizeLayerPalette     = NULL;
	qwglSetLayerPaletteEntries  = NULL;
	qwglShareLists              = NULL;
	qwglSwapLayerBuffers        = NULL;
	qwglUseFontBitmaps          = NULL;
	qwglUseFontOutlines         = NULL;

	qglAccum                    = NULL;
	qglAlphaFunc                = NULL;
	qglAreTexturesResident      = NULL;
	qglArrayElement             = NULL;
	qglBegin                    = NULL;
	qglBindTexture              = NULL;
	qglBitmap                   = NULL;
	qglBlendFunc                = NULL;
	qglCallList                 = NULL;
	qglCallLists                = NULL;
	qglClear                    = NULL;
	qglClearAccum               = NULL;
	qglClearColor               = NULL;
	qglClearDepth               = NULL;
	qglClearIndex               = NULL;
	qglClearStencil             = NULL;
	qglClipPlane                = NULL;
	qglColor3b                  = NULL;
	qglColor3bv                 = NULL;
	qglColor3d                  = NULL;
	qglColor3dv                 = NULL;
	qglColor3f                  = NULL;
	qglColor3fv                 = NULL;
	qglColor3i                  = NULL;
	qglColor3iv                 = NULL;
	qglColor3s                  = NULL;
	qglColor3sv                 = NULL;
	qglColor3ub                 = NULL;
	qglColor3ubv                = NULL;
	qglColor3ui                 = NULL;
	qglColor3uiv                = NULL;
	qglColor3us                 = NULL;
	qglColor3usv                = NULL;
	qglColor4b                  = NULL;
	qglColor4bv                 = NULL;
	qglColor4d                  = NULL;
	qglColor4dv                 = NULL;
	qglColor4f                  = NULL;
	qglColor4fv                 = NULL;
	qglColor4i                  = NULL;
	qglColor4iv                 = NULL;
	qglColor4s                  = NULL;
	qglColor4sv                 = NULL;
	qglColor4ub                 = NULL;
	qglColor4ubv                = NULL;
	qglColor4ui                 = NULL;
	qglColor4uiv                = NULL;
	qglColor4us                 = NULL;
	qglColor4usv                = NULL;
	qglColorMask                = NULL;
	qglColorMaterial            = NULL;
	qglColorPointer             = NULL;
	qglCopyPixels               = NULL;
	qglCopyTexImage1D           = NULL;
	qglCopyTexImage2D           = NULL;
	qglCopyTexSubImage1D        = NULL;
	qglCopyTexSubImage2D        = NULL;
	qglCullFace                 = NULL;
	qglDeleteLists              = NULL;
	qglDeleteTextures           = NULL;
	qglDepthFunc                = NULL;
	qglDepthMask                = NULL;
	qglDepthRange               = NULL;
	qglDisable                  = NULL;
	qglDisableClientState       = NULL;
	qglDrawArrays               = NULL;
	qglDrawBuffer               = NULL;
	qglDrawElements             = NULL;
	qglDrawPixels               = NULL;
	qglEdgeFlag                 = NULL;
	qglEdgeFlagPointer          = NULL;
	qglEdgeFlagv                = NULL;
	qglEnable                   = NULL;
	qglEnableClientState        = NULL;
	qglEnd                      = NULL;
	qglEndList                  = NULL;
	qglEvalCoord1d              = NULL;
	qglEvalCoord1dv             = NULL;
	qglEvalCoord1f              = NULL;
	qglEvalCoord1fv             = NULL;
	qglEvalCoord2d              = NULL;
	qglEvalCoord2dv             = NULL;
	qglEvalCoord2f              = NULL;
	qglEvalCoord2fv             = NULL;
	qglEvalMesh1                = NULL;
	qglEvalMesh2                = NULL;
	qglEvalPoint1               = NULL;
	qglEvalPoint2               = NULL;
	qglFeedbackBuffer           = NULL;
	qglFinish                   = NULL;
	qglFlush                    = NULL;
	qglFogf                     = NULL;
	qglFogfv                    = NULL;
	qglFogi                     = NULL;
	qglFogiv                    = NULL;
	qglFrontFace                = NULL;
	qglFrustum                  = NULL;
	qglGenLists                 = NULL;
	qglGenTextures              = NULL;
	qglGetBooleanv              = NULL;
	qglGetClipPlane             = NULL;
	qglGetDoublev               = NULL;
	qglGetError                 = NULL;
	qglGetFloatv                = NULL;
	qglGetIntegerv              = NULL;
	qglGetLightfv               = NULL;
	qglGetLightiv               = NULL;
	qglGetMapdv                 = NULL;
	qglGetMapfv                 = NULL;
	qglGetMapiv                 = NULL;
	qglGetMaterialfv            = NULL;
	qglGetMaterialiv            = NULL;
	qglGetPixelMapfv            = NULL;
	qglGetPixelMapuiv           = NULL;
	qglGetPixelMapusv           = NULL;
	qglGetPointerv              = NULL;
	qglGetPolygonStipple        = NULL;
	qglGetString                = NULL;
	qglGetTexEnvfv              = NULL;
	qglGetTexEnviv              = NULL;
	qglGetTexGendv              = NULL;
	qglGetTexGenfv              = NULL;
	qglGetTexGeniv              = NULL;
	qglGetTexImage              = NULL;
	qglGetTexLevelParameterfv   = NULL;
	qglGetTexLevelParameteriv   = NULL;
	qglGetTexParameterfv        = NULL;
	qglGetTexParameteriv        = NULL;
	qglHint                     = NULL;
	qglIndexMask                = NULL;
	qglIndexPointer             = NULL;
	qglIndexd                   = NULL;
	qglIndexdv                  = NULL;
	qglIndexf                   = NULL;
	qglIndexfv                  = NULL;
	qglIndexi                   = NULL;
	qglIndexiv                  = NULL;
	qglIndexs                   = NULL;
	qglIndexsv                  = NULL;
	qglIndexub                  = NULL;
	qglIndexubv                 = NULL;
	qglInitNames                = NULL;
	qglInterleavedArrays        = NULL;
	qglIsEnabled                = NULL;
	qglIsList                   = NULL;
	qglIsTexture                = NULL;
	qglLightModelf              = NULL;
	qglLightModelfv             = NULL;
	qglLightModeli              = NULL;
	qglLightModeliv             = NULL;
	qglLightf                   = NULL;
	qglLightfv                  = NULL;
	qglLighti                   = NULL;
	qglLightiv                  = NULL;
	qglLineStipple              = NULL;
	qglLineWidth                = NULL;
	qglListBase                 = NULL;
	qglLoadIdentity             = NULL;
	qglLoadMatrixd              = NULL;
	qglLoadMatrixf              = NULL;
	qglLoadName                 = NULL;
	qglLogicOp                  = NULL;
	qglMap1d                    = NULL;
	qglMap1f                    = NULL;
	qglMap2d                    = NULL;
	qglMap2f                    = NULL;
	qglMapGrid1d                = NULL;
	qglMapGrid1f                = NULL;
	qglMapGrid2d                = NULL;
	qglMapGrid2f                = NULL;
	qglMaterialf                = NULL;
	qglMaterialfv               = NULL;
	qglMateriali                = NULL;
	qglMaterialiv               = NULL;
	qglMatrixMode               = NULL;
	qglMultMatrixd              = NULL;
	qglMultMatrixf              = NULL;
	qglNewList                  = NULL;
	qglNormal3b                 = NULL;
	qglNormal3bv                = NULL;
	qglNormal3d                 = NULL;
	qglNormal3dv                = NULL;
	qglNormal3f                 = NULL;
	qglNormal3fv                = NULL;
	qglNormal3i                 = NULL;
	qglNormal3iv                = NULL;
	qglNormal3s                 = NULL;
	qglNormal3sv                = NULL;
	qglNormalPointer            = NULL;
	qglOrtho                    = NULL;
	qglPassThrough              = NULL;
	qglPixelMapfv               = NULL;
	qglPixelMapuiv              = NULL;
	qglPixelMapusv              = NULL;
	qglPixelStoref              = NULL;
	qglPixelStorei              = NULL;
	qglPixelTransferf           = NULL;
	qglPixelTransferi           = NULL;
	qglPixelZoom                = NULL;
	qglPointSize                = NULL;
	qglPolygonMode              = NULL;
	qglPolygonOffset            = NULL;
	qglPolygonStipple           = NULL;
	qglPopAttrib                = NULL;
	qglPopClientAttrib          = NULL;
	qglPopMatrix                = NULL;
	qglPopName                  = NULL;
	qglPrioritizeTextures       = NULL;
	qglPushAttrib               = NULL;
	qglPushClientAttrib         = NULL;
	qglPushMatrix               = NULL;
	qglPushName                 = NULL;
	qglRasterPos2d              = NULL;
	qglRasterPos2dv             = NULL;
	qglRasterPos2f              = NULL;
	qglRasterPos2fv             = NULL;
	qglRasterPos2i              = NULL;
	qglRasterPos2iv             = NULL;
	qglRasterPos2s              = NULL;
	qglRasterPos2sv             = NULL;
	qglRasterPos3d              = NULL;
	qglRasterPos3dv             = NULL;
	qglRasterPos3f              = NULL;
	qglRasterPos3fv             = NULL;
	qglRasterPos3i              = NULL;
	qglRasterPos3iv             = NULL;
	qglRasterPos3s              = NULL;
	qglRasterPos3sv             = NULL;
	qglRasterPos4d              = NULL;
	qglRasterPos4dv             = NULL;
	qglRasterPos4f              = NULL;
	qglRasterPos4fv             = NULL;
	qglRasterPos4i              = NULL;
	qglRasterPos4iv             = NULL;
	qglRasterPos4s              = NULL;
	qglRasterPos4sv             = NULL;
	qglReadBuffer               = NULL;
	qglReadPixels               = NULL;
	qglRectd                    = NULL;
	qglRectdv                   = NULL;
	qglRectf                    = NULL;
	qglRectfv                   = NULL;
	qglRecti                    = NULL;
	qglRectiv                   = NULL;
	qglRects                    = NULL;
	qglRectsv                   = NULL;
	qglRenderMode               = NULL;
	qglRotated                  = NULL;
	qglRotatef                  = NULL;
	qglScaled                   = NULL;
	qglScalef                   = NULL;
	qglScissor                  = NULL;
	qglSelectBuffer             = NULL;
	qglShadeModel               = NULL;
	qglStencilFunc              = NULL;
	qglStencilMask              = NULL;
	qglStencilOp                = NULL;
	qglTexCoord1d               = NULL;
	qglTexCoord1dv              = NULL;
	qglTexCoord1f               = NULL;
	qglTexCoord1fv              = NULL;
	qglTexCoord1i               = NULL;
	qglTexCoord1iv              = NULL;
	qglTexCoord1s               = NULL;
	qglTexCoord1sv              = NULL;
	qglTexCoord2d               = NULL;
	qglTexCoord2dv              = NULL;
	qglTexCoord2f               = NULL;
	qglTexCoord2fv              = NULL;
	qglTexCoord2i               = NULL;
	qglTexCoord2iv              = NULL;
	qglTexCoord2s               = NULL;
	qglTexCoord2sv              = NULL;
	qglTexCoord3d               = NULL;
	qglTexCoord3dv              = NULL;
	qglTexCoord3f               = NULL;
	qglTexCoord3fv              = NULL;
	qglTexCoord3i               = NULL;
	qglTexCoord3iv              = NULL;
	qglTexCoord3s               = NULL;
	qglTexCoord3sv              = NULL;
	qglTexCoord4d               = NULL;
	qglTexCoord4dv              = NULL;
	qglTexCoord4f               = NULL;
	qglTexCoord4fv              = NULL;
	qglTexCoord4i               = NULL;
	qglTexCoord4iv              = NULL;
	qglTexCoord4s               = NULL;
	qglTexCoord4sv              = NULL;
	qglTexCoordPointer          = NULL;
	qglTexEnvf                  = NULL;
	qglTexEnvfv                 = NULL;
	qglTexEnvi                  = NULL;
	qglTexEnviv                 = NULL;
	qglTexGend                  = NULL;
	qglTexGendv                 = NULL;
	qglTexGenf                  = NULL;
	qglTexGenfv                 = NULL;
	qglTexGeni                  = NULL;
	qglTexGeniv                 = NULL;
	qglTexImage1D               = NULL;
	qglTexImage2D               = NULL;
	qglTexParameterf            = NULL;
	qglTexParameterfv           = NULL;
	qglTexParameteri            = NULL;
	qglTexParameteriv           = NULL;
	qglTexSubImage1D            = NULL;
	qglTexSubImage2D            = NULL;
	qglTranslated               = NULL;
	qglTranslatef               = NULL;
	qglVertex2d                 = NULL;
	qglVertex2dv                = NULL;
	qglVertex2f                 = NULL;
	qglVertex2fv                = NULL;
	qglVertex2i                 = NULL;
	qglVertex2iv                = NULL;
	qglVertex2s                 = NULL;
	qglVertex2sv                = NULL;
	qglVertex3d                 = NULL;
	qglVertex3dv                = NULL;
	qglVertex3f                 = NULL;
	qglVertex3fv                = NULL;
	qglVertex3i                 = NULL;
	qglVertex3iv                = NULL;
	qglVertex3s                 = NULL;
	qglVertex3sv                = NULL;
	qglVertex4d                 = NULL;
	qglVertex4dv                = NULL;
	qglVertex4f                 = NULL;
	qglVertex4fv                = NULL;
	qglVertex4i                 = NULL;
	qglVertex4iv                = NULL;
	qglVertex4s                 = NULL;
	qglVertex4sv                = NULL;
	qglVertexPointer            = NULL;
	qglViewport                 = NULL;

	qglActiveTextureARB					= NULL;
	qglClientActiveTextureARB			= NULL;

	qglBindBufferARB					= NULL;
	qglDeleteBuffersARB					= NULL;
	qglGenBuffersARB					= NULL;
	qglBufferDataARB					= NULL;
	qglBufferSubDataARB					= NULL;
	qglMapBufferARB						= NULL;
	qglUnmapBufferARB					= NULL;

	qglVertexAttribPointerARB			= NULL;
	qglEnableVertexAttribArrayARB		= NULL;
	qglDisableVertexAttribArrayARB		= NULL;
	qglBindProgramARB					= NULL;
	qglDeleteProgramsARB				= NULL;
	qglGenProgramsARB					= NULL;
	qglProgramStringARB					= NULL;
	qglProgramEnvParameter4fARB			= NULL;
	qglProgramEnvParameter4fvARB		= NULL;
	qglProgramLocalParameter4fARB		= NULL;
	qglProgramLocalParameter4fvARB		= NULL;
	qglGetProgramivARB					= NULL;

	qglDrawRangeElementsEXT				= NULL;

	qglActiveStencilFaceEXT				= NULL;

	qglDepthBoundsEXT					= NULL;

	qwglSwapIntervalEXT					= NULL;

	qglCombinerParameterfvNV			= NULL;
	qglCombinerParameterfNV				= NULL;
	qglCombinerParameterivNV			= NULL;
	qglCombinerParameteriNV				= NULL;
	qglCombinerInputNV					= NULL;
	qglCombinerOutputNV					= NULL;
	qglFinalCombinerInputNV				= NULL;

	qglGenFragmentShadersATI			= NULL;
	qglBindFragmentShaderATI			= NULL;
	qglDeleteFragmentShaderATI			= NULL;
	qglBeginFragmentShaderATI			= NULL;
	qglEndFragmentShaderATI				= NULL;
	qglPassTexCoordATI					= NULL;
	qglSampleMapATI						= NULL;
	qglColorFragmentOp1ATI				= NULL;
	qglColorFragmentOp2ATI				= NULL;
	qglColorFragmentOp3ATI				= NULL;
	qglAlphaFragmentOp1ATI				= NULL;
	qglAlphaFragmentOp2ATI				= NULL;
	qglAlphaFragmentOp3ATI				= NULL;
	qglSetFragmentShaderConstantATI		= NULL;

	qglStencilOpSeparateATI				= NULL;
	qglStencilFuncSeparateATI			= NULL;
}

/*
 =================
 QGL_Init

 Binds our QGL function pointers to the appropriate GL stuff
 =================
*/
qboolean QGL_Init (const char *driver){

	char	name[MAX_OSPATH], path[MAX_OSPATH];
	Com_Printf("...initializing QGL\n");

	qglExtensionPointers = false;

	Q_snprintfz(name, sizeof(name), "%s.dll", driver);
	if (!SearchPath(NULL, name, NULL, sizeof(path), path, NULL)){
		Com_Printf("...WARNING: couldn't find OpenGL driver '%s'\n", name);
		return false;
	}

	Com_Printf("...calling LoadLibrary( '%s' ): ", path);
	if ((glwState.hInstOpenGL = LoadLibrary(path)) == NULL){
		Com_Printf("failed\n");
		return false;
	}
	Com_Printf("succeeded\n");

	qwglChoosePixelFormat       = QGL_GetProcAddress("wglChoosePixelFormat");
	qwglDescribePixelFormat     = QGL_GetProcAddress("wglDescribePixelFormat");
	qwglGetPixelFormat          = QGL_GetProcAddress("wglGetPixelFormat");
	qwglSetPixelFormat          = QGL_GetProcAddress("wglSetPixelFormat");
	qwglSwapBuffers             = QGL_GetProcAddress("wglSwapBuffers");

	qwglCopyContext             = QGL_GetProcAddress("wglCopyContext");
	qwglCreateContext           = QGL_GetProcAddress("wglCreateContext");
	qwglCreateLayerContext      = QGL_GetProcAddress("wglCreateLayerContext");
	qwglDeleteContext           = QGL_GetProcAddress("wglDeleteContext");
	qwglDescribeLayerPlane      = QGL_GetProcAddress("wglDescribeLayerPlane");
	qwglGetCurrentContext       = QGL_GetProcAddress("wglGetCurrentContext");
	qwglGetCurrentDC            = QGL_GetProcAddress("wglGetCurrentDC");
	qwglGetLayerPaletteEntries  = QGL_GetProcAddress("wglGetLayerPaletteEntries");
	qwglGetProcAddress          = QGL_GetProcAddress("wglGetProcAddress");
	qwglMakeCurrent             = QGL_GetProcAddress("wglMakeCurrent");
	qwglRealizeLayerPalette     = QGL_GetProcAddress("wglRealizeLayerPalette");
	qwglSetLayerPaletteEntries  = QGL_GetProcAddress("wglSetLayerPaletteEntries");
	qwglShareLists              = QGL_GetProcAddress("wglShareLists");
	qwglSwapLayerBuffers        = QGL_GetProcAddress("wglSwapLayerBuffers");
	qwglUseFontBitmaps          = QGL_GetProcAddress("wglUseFontBitmapsA");
	qwglUseFontOutlines         = QGL_GetProcAddress("wglUseFontOutlinesA");

	qglAccum					= dllAccum						= QGL_GetProcAddress("glAccum");
	qglAlphaFunc                = dllAlphaFunc					= QGL_GetProcAddress("glAlphaFunc");
	qglAreTexturesResident      = dllAreTexturesResident		= QGL_GetProcAddress("glAreTexturesResident");
	qglArrayElement             = dllArrayElement				= QGL_GetProcAddress("glArrayElement");
	qglBegin                    = dllBegin						= QGL_GetProcAddress("glBegin");
	qglBindTexture              = dllBindTexture				= QGL_GetProcAddress("glBindTexture");
	qglBitmap                   = dllBitmap						= QGL_GetProcAddress("glBitmap");
	qglBlendFunc                = dllBlendFunc					= QGL_GetProcAddress("glBlendFunc");
	qglCallList                 = dllCallList					= QGL_GetProcAddress("glCallList");
	qglCallLists                = dllCallLists					= QGL_GetProcAddress("glCallLists");
	qglClear                    = dllClear						= QGL_GetProcAddress("glClear");
	qglClearAccum               = dllClearAccum					= QGL_GetProcAddress("glClearAccum");
	qglClearColor               = dllClearColor					= QGL_GetProcAddress("glClearColor");
	qglClearDepth               = dllClearDepth					= QGL_GetProcAddress("glClearDepth");
	qglClearIndex               = dllClearIndex					= QGL_GetProcAddress("glClearIndex");
	qglClearStencil             = dllClearStencil				= QGL_GetProcAddress("glClearStencil");
	qglClipPlane                = dllClipPlane					= QGL_GetProcAddress("glClipPlane");
	qglColor3b                  = dllColor3b					= QGL_GetProcAddress("glColor3b");
	qglColor3bv                 = dllColor3bv					= QGL_GetProcAddress("glColor3bv");
	qglColor3d                  = dllColor3d					= QGL_GetProcAddress("glColor3d");
	qglColor3dv                 = dllColor3dv					= QGL_GetProcAddress("glColor3dv");
	qglColor3f                  = dllColor3f					= QGL_GetProcAddress("glColor3f");
	qglColor3fv                 = dllColor3fv					= QGL_GetProcAddress("glColor3fv");
	qglColor3i                  = dllColor3i					= QGL_GetProcAddress("glColor3i");
	qglColor3iv                 = dllColor3iv					= QGL_GetProcAddress("glColor3iv");
	qglColor3s                  = dllColor3s					= QGL_GetProcAddress("glColor3s");
	qglColor3sv                 = dllColor3sv					= QGL_GetProcAddress("glColor3sv");
	qglColor3ub                 = dllColor3ub					= QGL_GetProcAddress("glColor3ub");
	qglColor3ubv                = dllColor3ubv					= QGL_GetProcAddress("glColor3ubv");
	qglColor3ui                 = dllColor3ui					= QGL_GetProcAddress("glColor3ui");
	qglColor3uiv                = dllColor3uiv					= QGL_GetProcAddress("glColor3uiv");
	qglColor3us                 = dllColor3us					= QGL_GetProcAddress("glColor3us");
	qglColor3usv                = dllColor3usv					= QGL_GetProcAddress("glColor3usv");
	qglColor4b                  = dllColor4b					= QGL_GetProcAddress("glColor4b");
	qglColor4bv                 = dllColor4bv					= QGL_GetProcAddress("glColor4bv");
	qglColor4d                  = dllColor4d					= QGL_GetProcAddress("glColor4d");
	qglColor4dv                 = dllColor4dv					= QGL_GetProcAddress("glColor4dv");
	qglColor4f                  = dllColor4f					= QGL_GetProcAddress("glColor4f");
	qglColor4fv                 = dllColor4fv					= QGL_GetProcAddress("glColor4fv");
	qglColor4i                  = dllColor4i					= QGL_GetProcAddress("glColor4i");
	qglColor4iv                 = dllColor4iv					= QGL_GetProcAddress("glColor4iv");
	qglColor4s                  = dllColor4s					= QGL_GetProcAddress("glColor4s");
	qglColor4sv                 = dllColor4sv					= QGL_GetProcAddress("glColor4sv");
	qglColor4ub                 = dllColor4ub					= QGL_GetProcAddress("glColor4ub");
	qglColor4ubv                = dllColor4ubv					= QGL_GetProcAddress("glColor4ubv");
	qglColor4ui                 = dllColor4ui					= QGL_GetProcAddress("glColor4ui");
	qglColor4uiv                = dllColor4uiv					= QGL_GetProcAddress("glColor4uiv");
	qglColor4us                 = dllColor4us					= QGL_GetProcAddress("glColor4us");
	qglColor4usv                = dllColor4usv					= QGL_GetProcAddress("glColor4usv");
	qglColorMask                = dllColorMask					= QGL_GetProcAddress("glColorMask");
	qglColorMaterial            = dllColorMaterial				= QGL_GetProcAddress("glColorMaterial");
	qglColorPointer             = dllColorPointer				= QGL_GetProcAddress("glColorPointer");
	qglCopyPixels               = dllCopyPixels					= QGL_GetProcAddress("glCopyPixels");
	qglCopyTexImage1D           = dllCopyTexImage1D				= QGL_GetProcAddress("glCopyTexImage1D");
	qglCopyTexImage2D           = dllCopyTexImage2D				= QGL_GetProcAddress("glCopyTexImage2D");
	qglCopyTexSubImage1D        = dllCopyTexSubImage1D			= QGL_GetProcAddress("glCopyTexSubImage1D");
	qglCopyTexSubImage2D        = dllCopyTexSubImage2D			= QGL_GetProcAddress("glCopyTexSubImage2D");
	qglCullFace                 = dllCullFace					= QGL_GetProcAddress("glCullFace");
	qglDeleteLists              = dllDeleteLists				= QGL_GetProcAddress("glDeleteLists");
	qglDeleteTextures           = dllDeleteTextures				= QGL_GetProcAddress("glDeleteTextures");
	qglDepthFunc                = dllDepthFunc					= QGL_GetProcAddress("glDepthFunc");
	qglDepthMask                = dllDepthMask					= QGL_GetProcAddress("glDepthMask");
	qglDepthRange               = dllDepthRange					= QGL_GetProcAddress("glDepthRange");
	qglDisable                  = dllDisable					= QGL_GetProcAddress("glDisable");
	qglDisableClientState       = dllDisableClientState			= QGL_GetProcAddress("glDisableClientState");
	qglDrawArrays               = dllDrawArrays					= QGL_GetProcAddress("glDrawArrays");
	qglDrawBuffer               = dllDrawBuffer					= QGL_GetProcAddress("glDrawBuffer");
	qglDrawElements				= dllDrawElements				= QGL_GetProcAddress("glDrawElements");
	qglDrawPixels				= dllDrawPixels					= QGL_GetProcAddress("glDrawPixels");
	qglEdgeFlag                 = dllEdgeFlag					= QGL_GetProcAddress("glEdgeFlag");
	qglEdgeFlagPointer          = dllEdgeFlagPointer			= QGL_GetProcAddress("glEdgeFlagPointer");
	qglEdgeFlagv                = dllEdgeFlagv					= QGL_GetProcAddress("glEdgeFlagv");
	qglEnable                   = dllEnable						= QGL_GetProcAddress("glEnable");
	qglEnableClientState        = dllEnableClientState			= QGL_GetProcAddress("glEnableClientState");
	qglEnd                      = dllEnd						= QGL_GetProcAddress("glEnd");
	qglEndList                  = dllEndList					= QGL_GetProcAddress("glEndList");
	qglEvalCoord1d				= dllEvalCoord1d				= QGL_GetProcAddress("glEvalCoord1d");
	qglEvalCoord1dv             = dllEvalCoord1dv				= QGL_GetProcAddress("glEvalCoord1dv");
	qglEvalCoord1f              = dllEvalCoord1f				= QGL_GetProcAddress("glEvalCoord1f");
	qglEvalCoord1fv             = dllEvalCoord1fv				= QGL_GetProcAddress("glEvalCoord1fv");
	qglEvalCoord2d              = dllEvalCoord2d				= QGL_GetProcAddress("glEvalCoord2d");
	qglEvalCoord2dv             = dllEvalCoord2dv				= QGL_GetProcAddress("glEvalCoord2dv");
	qglEvalCoord2f              = dllEvalCoord2f				= QGL_GetProcAddress("glEvalCoord2f");
	qglEvalCoord2fv             = dllEvalCoord2fv				= QGL_GetProcAddress("glEvalCoord2fv");
	qglEvalMesh1                = dllEvalMesh1					= QGL_GetProcAddress("glEvalMesh1");
	qglEvalMesh2                = dllEvalMesh2					= QGL_GetProcAddress("glEvalMesh2");
	qglEvalPoint1               = dllEvalPoint1					= QGL_GetProcAddress("glEvalPoint1");
	qglEvalPoint2               = dllEvalPoint2					= QGL_GetProcAddress("glEvalPoint2");
	qglFeedbackBuffer           = dllFeedbackBuffer				= QGL_GetProcAddress("glFeedbackBuffer");
	qglFinish                   = dllFinish						= QGL_GetProcAddress("glFinish");
	qglFlush                    = dllFlush						= QGL_GetProcAddress("glFlush");
	qglFogf                     = dllFogf						= QGL_GetProcAddress("glFogf");
	qglFogfv                    = dllFogfv						= QGL_GetProcAddress("glFogfv");
	qglFogi                     = dllFogi						= QGL_GetProcAddress("glFogi");
	qglFogiv                    = dllFogiv						= QGL_GetProcAddress("glFogiv");
	qglFrontFace                = dllFrontFace					= QGL_GetProcAddress("glFrontFace");
	qglFrustum                  = dllFrustum					= QGL_GetProcAddress("glFrustum");
	qglGenLists                 = dllGenLists					= QGL_GetProcAddress("glGenLists");
	qglGenTextures              = dllGenTextures				= QGL_GetProcAddress("glGenTextures");
	qglGetBooleanv              = dllGetBooleanv				= QGL_GetProcAddress("glGetBooleanv");
	qglGetClipPlane             = dllGetClipPlane				= QGL_GetProcAddress("glGetClipPlane");
	qglGetDoublev               = dllGetDoublev					= QGL_GetProcAddress("glGetDoublev");
	qglGetError                 = dllGetError					= QGL_GetProcAddress("glGetError");
	qglGetFloatv                = dllGetFloatv					= QGL_GetProcAddress("glGetFloatv");
	qglGetIntegerv              = dllGetIntegerv				= QGL_GetProcAddress("glGetIntegerv");
	qglGetLightfv               = dllGetLightfv					= QGL_GetProcAddress("glGetLightfv");
	qglGetLightiv               = dllGetLightiv					= QGL_GetProcAddress("glGetLightiv");
	qglGetMapdv                 = dllGetMapdv					= QGL_GetProcAddress("glGetMapdv");
	qglGetMapfv                 = dllGetMapfv					= QGL_GetProcAddress("glGetMapfv");
	qglGetMapiv                 = dllGetMapiv					= QGL_GetProcAddress("glGetMapiv");
	qglGetMaterialfv            = dllGetMaterialfv				= QGL_GetProcAddress("glGetMaterialfv");
	qglGetMaterialiv            = dllGetMaterialiv				= QGL_GetProcAddress("glGetMaterialiv");
	qglGetPixelMapfv			= dllGetPixelMapfv				= QGL_GetProcAddress("glGetPixelMapfv");
	qglGetPixelMapuiv           = dllGetPixelMapuiv				= QGL_GetProcAddress("glGetPixelMapuiv");
	qglGetPixelMapusv           = dllGetPixelMapusv				= QGL_GetProcAddress("glGetPixelMapusv");
	qglGetPointerv              = dllGetPointerv				= QGL_GetProcAddress("glGetPointerv");
	qglGetPolygonStipple        = dllGetPolygonStipple			= QGL_GetProcAddress("glGetPolygonStipple");
	qglGetString                = dllGetString					= QGL_GetProcAddress("glGetString");
	qglGetTexEnvfv              = dllGetTexEnvfv				= QGL_GetProcAddress("glGetTexEnvfv");
	qglGetTexEnviv              = dllGetTexEnviv				= QGL_GetProcAddress("glGetTexEnviv");
	qglGetTexGendv              = dllGetTexGendv				= QGL_GetProcAddress("glGetTexGendv");
	qglGetTexGenfv              = dllGetTexGenfv				= QGL_GetProcAddress("glGetTexGenfv");
	qglGetTexGeniv              = dllGetTexGeniv				= QGL_GetProcAddress("glGetTexGeniv");
	qglGetTexImage              = dllGetTexImage				= QGL_GetProcAddress("glGetTexImage");
	qglGetTexLevelParameterfv   = dllGetTexLevelParameterfv		= QGL_GetProcAddress("glGetTexLevelParameterfv");
	qglGetTexLevelParameteriv   = dllGetTexLevelParameteriv		= QGL_GetProcAddress("glGetTexLevelParameteriv");
	qglGetTexParameterfv        = dllGetTexParameterfv			= QGL_GetProcAddress("glGetTexParameterfv");
	qglGetTexParameteriv        = dllGetTexParameteriv			= QGL_GetProcAddress("glGetTexParameteriv");
	qglHint                     = dllHint						= QGL_GetProcAddress("glHint");
	qglIndexMask                = dllIndexMask					= QGL_GetProcAddress("glIndexMask");
	qglIndexPointer             = dllIndexPointer				= QGL_GetProcAddress("glIndexPointer");
	qglIndexd                   = dllIndexd						= QGL_GetProcAddress("glIndexd");
	qglIndexdv                  = dllIndexdv					= QGL_GetProcAddress("glIndexdv");
	qglIndexf                   = dllIndexf						= QGL_GetProcAddress("glIndexf");
	qglIndexfv                  = dllIndexfv					= QGL_GetProcAddress("glIndexfv");
	qglIndexi                   = dllIndexi						= QGL_GetProcAddress("glIndexi");
	qglIndexiv                  = dllIndexiv					= QGL_GetProcAddress("glIndexiv");
	qglIndexs                   = dllIndexs						= QGL_GetProcAddress("glIndexs");
	qglIndexsv                  = dllIndexsv					= QGL_GetProcAddress("glIndexsv");
	qglIndexub                  = dllIndexub					= QGL_GetProcAddress("glIndexub");
	qglIndexubv                 = dllIndexubv					= QGL_GetProcAddress("glIndexubv");
	qglInitNames                = dllInitNames					= QGL_GetProcAddress("glInitNames");
	qglInterleavedArrays        = dllInterleavedArrays			= QGL_GetProcAddress("glInterleavedArrays");
	qglIsEnabled                = dllIsEnabled					= QGL_GetProcAddress("glIsEnabled");
	qglIsList                   = dllIsList						= QGL_GetProcAddress("glIsList");
	qglIsTexture                = dllIsTexture					= QGL_GetProcAddress("glIsTexture");
	qglLightModelf              = dllLightModelf				= QGL_GetProcAddress("glLightModelf");
	qglLightModelfv             = dllLightModelfv				= QGL_GetProcAddress("glLightModelfv");
	qglLightModeli              = dllLightModeli				= QGL_GetProcAddress("glLightModeli");
	qglLightModeliv             = dllLightModeliv				= QGL_GetProcAddress("glLightModeliv");
	qglLightf                   = dllLightf						= QGL_GetProcAddress("glLightf");
	qglLightfv                  = dllLightfv					= QGL_GetProcAddress("glLightfv");
	qglLighti                   = dllLighti						= QGL_GetProcAddress("glLighti");
	qglLightiv                  = dllLightiv					= QGL_GetProcAddress("glLightiv");
	qglLineStipple              = dllLineStipple				= QGL_GetProcAddress("glLineStipple");
	qglLineWidth                = dllLineWidth					= QGL_GetProcAddress("glLineWidth");
	qglListBase                 = dllListBase					= QGL_GetProcAddress("glListBase");
	qglLoadIdentity             = dllLoadIdentity				= QGL_GetProcAddress("glLoadIdentity");
	qglLoadMatrixd              = dllLoadMatrixd				= QGL_GetProcAddress("glLoadMatrixd");
	qglLoadMatrixf              = dllLoadMatrixf				= QGL_GetProcAddress("glLoadMatrixf");
	qglLoadName                 = dllLoadName					= QGL_GetProcAddress("glLoadName");
	qglLogicOp                  = dllLogicOp					= QGL_GetProcAddress("glLogicOp");
	qglMap1d                    = dllMap1d						= QGL_GetProcAddress("glMap1d");
	qglMap1f                    = dllMap1f						= QGL_GetProcAddress("glMap1f");
	qglMap2d                    = dllMap2d						= QGL_GetProcAddress("glMap2d");
	qglMap2f                    = dllMap2f						= QGL_GetProcAddress("glMap2f");
	qglMapGrid1d                = dllMapGrid1d					= QGL_GetProcAddress("glMapGrid1d");
	qglMapGrid1f                = dllMapGrid1f					= QGL_GetProcAddress("glMapGrid1f");
	qglMapGrid2d                = dllMapGrid2d					= QGL_GetProcAddress("glMapGrid2d");
	qglMapGrid2f                = dllMapGrid2f					= QGL_GetProcAddress("glMapGrid2f");
	qglMaterialf                = dllMaterialf					= QGL_GetProcAddress("glMaterialf");
	qglMaterialfv               = dllMaterialfv					= QGL_GetProcAddress("glMaterialfv");
	qglMateriali                = dllMateriali					= QGL_GetProcAddress("glMateriali");
	qglMaterialiv               = dllMaterialiv					= QGL_GetProcAddress("glMaterialiv");
	qglMatrixMode               = dllMatrixMode					= QGL_GetProcAddress("glMatrixMode");
	qglMultMatrixd              = dllMultMatrixd				= QGL_GetProcAddress("glMultMatrixd");
	qglMultMatrixf              = dllMultMatrixf				= QGL_GetProcAddress("glMultMatrixf");
	qglNewList                  = dllNewList					= QGL_GetProcAddress("glNewList");
	qglNormal3b                 = dllNormal3b					= QGL_GetProcAddress("glNormal3b");
	qglNormal3bv                = dllNormal3bv					= QGL_GetProcAddress("glNormal3bv");
	qglNormal3d                 = dllNormal3d					= QGL_GetProcAddress("glNormal3d");
	qglNormal3dv                = dllNormal3dv					= QGL_GetProcAddress("glNormal3dv");
	qglNormal3f                 = dllNormal3f					= QGL_GetProcAddress("glNormal3f");
	qglNormal3fv                = dllNormal3fv					= QGL_GetProcAddress("glNormal3fv");
	qglNormal3i                 = dllNormal3i					= QGL_GetProcAddress("glNormal3i");
	qglNormal3iv                = dllNormal3iv					= QGL_GetProcAddress("glNormal3iv");
	qglNormal3s                 = dllNormal3s					= QGL_GetProcAddress("glNormal3s");
	qglNormal3sv                = dllNormal3sv					= QGL_GetProcAddress("glNormal3sv");
	qglNormalPointer            = dllNormalPointer				= QGL_GetProcAddress("glNormalPointer");
	qglOrtho                    = dllOrtho						= QGL_GetProcAddress("glOrtho");
	qglPassThrough              = dllPassThrough				= QGL_GetProcAddress("glPassThrough");
	qglPixelMapfv               = dllPixelMapfv					= QGL_GetProcAddress("glPixelMapfv");
	qglPixelMapuiv              = dllPixelMapuiv				= QGL_GetProcAddress("glPixelMapuiv");
	qglPixelMapusv              = dllPixelMapusv				= QGL_GetProcAddress("glPixelMapusv");
	qglPixelStoref              = dllPixelStoref				= QGL_GetProcAddress("glPixelStoref");
	qglPixelStorei              = dllPixelStorei				= QGL_GetProcAddress("glPixelStorei");
	qglPixelTransferf           = dllPixelTransferf				= QGL_GetProcAddress("glPixelTransferf");
	qglPixelTransferi           = dllPixelTransferi				= QGL_GetProcAddress("glPixelTransferi");
	qglPixelZoom                = dllPixelZoom					= QGL_GetProcAddress("glPixelZoom");
	qglPointSize                = dllPointSize					= QGL_GetProcAddress("glPointSize");
	qglPolygonMode              = dllPolygonMode				= QGL_GetProcAddress("glPolygonMode");
	qglPolygonOffset            = dllPolygonOffset				= QGL_GetProcAddress("glPolygonOffset");
	qglPolygonStipple           = dllPolygonStipple				= QGL_GetProcAddress("glPolygonStipple");
	qglPopAttrib                = dllPopAttrib					= QGL_GetProcAddress("glPopAttrib");
	qglPopClientAttrib          = dllPopClientAttrib			= QGL_GetProcAddress("glPopClientAttrib");
	qglPopMatrix                = dllPopMatrix					= QGL_GetProcAddress("glPopMatrix");
	qglPopName                  = dllPopName					= QGL_GetProcAddress("glPopName");
	qglPrioritizeTextures       = dllPrioritizeTextures			= QGL_GetProcAddress("glPrioritizeTextures");
	qglPushAttrib               = dllPushAttrib					= QGL_GetProcAddress("glPushAttrib");
	qglPushClientAttrib         = dllPushClientAttrib			= QGL_GetProcAddress("glPushClientAttrib");
	qglPushMatrix               = dllPushMatrix					= QGL_GetProcAddress("glPushMatrix");
	qglPushName                 = dllPushName					= QGL_GetProcAddress("glPushName");
	qglRasterPos2d              = dllRasterPos2d				= QGL_GetProcAddress("glRasterPos2d");
	qglRasterPos2dv             = dllRasterPos2dv				= QGL_GetProcAddress("glRasterPos2dv");
	qglRasterPos2f              = dllRasterPos2f				= QGL_GetProcAddress("glRasterPos2f");
	qglRasterPos2fv             = dllRasterPos2fv				= QGL_GetProcAddress("glRasterPos2fv");
	qglRasterPos2i              = dllRasterPos2i				= QGL_GetProcAddress("glRasterPos2i");
	qglRasterPos2iv             = dllRasterPos2iv				= QGL_GetProcAddress("glRasterPos2iv");
	qglRasterPos2s              = dllRasterPos2s				= QGL_GetProcAddress("glRasterPos2s");
	qglRasterPos2sv             = dllRasterPos2sv				= QGL_GetProcAddress("glRasterPos2sv");
	qglRasterPos3d              = dllRasterPos3d				= QGL_GetProcAddress("glRasterPos3d");
	qglRasterPos3dv             = dllRasterPos3dv				= QGL_GetProcAddress("glRasterPos3dv");
	qglRasterPos3f              = dllRasterPos3f				= QGL_GetProcAddress("glRasterPos3f");
	qglRasterPos3fv             = dllRasterPos3fv				= QGL_GetProcAddress("glRasterPos3fv");
	qglRasterPos3i              = dllRasterPos3i				= QGL_GetProcAddress("glRasterPos3i");
	qglRasterPos3iv             = dllRasterPos3iv				= QGL_GetProcAddress("glRasterPos3iv");
	qglRasterPos3s              = dllRasterPos3s				= QGL_GetProcAddress("glRasterPos3s");
	qglRasterPos3sv             = dllRasterPos3sv				= QGL_GetProcAddress("glRasterPos3sv");
	qglRasterPos4d              = dllRasterPos4d				= QGL_GetProcAddress("glRasterPos4d");
	qglRasterPos4dv             = dllRasterPos4dv				= QGL_GetProcAddress("glRasterPos4dv");
	qglRasterPos4f              = dllRasterPos4f				= QGL_GetProcAddress("glRasterPos4f");
	qglRasterPos4fv             = dllRasterPos4fv				= QGL_GetProcAddress("glRasterPos4fv");
	qglRasterPos4i              = dllRasterPos4i				= QGL_GetProcAddress("glRasterPos4i");
	qglRasterPos4iv             = dllRasterPos4iv				= QGL_GetProcAddress("glRasterPos4iv");
	qglRasterPos4s              = dllRasterPos4s				= QGL_GetProcAddress("glRasterPos4s");
	qglRasterPos4sv             = dllRasterPos4sv				= QGL_GetProcAddress("glRasterPos4sv");
	qglReadBuffer               = dllReadBuffer					= QGL_GetProcAddress("glReadBuffer");
	qglReadPixels               = dllReadPixels					= QGL_GetProcAddress("glReadPixels");
	qglRectd                    = dllRectd						= QGL_GetProcAddress("glRectd");
	qglRectdv                   = dllRectdv						= QGL_GetProcAddress("glRectdv");
	qglRectf                    = dllRectf						= QGL_GetProcAddress("glRectf");
	qglRectfv                   = dllRectfv						= QGL_GetProcAddress("glRectfv");
	qglRecti                    = dllRecti						= QGL_GetProcAddress("glRecti");
	qglRectiv                   = dllRectiv						= QGL_GetProcAddress("glRectiv");
	qglRects                    = dllRects						= QGL_GetProcAddress("glRects");
	qglRectsv                   = dllRectsv						= QGL_GetProcAddress("glRectsv");
	qglRenderMode               = dllRenderMode					= QGL_GetProcAddress("glRenderMode");
	qglRotated                  = dllRotated					= QGL_GetProcAddress("glRotated");
	qglRotatef                  = dllRotatef					= QGL_GetProcAddress("glRotatef");
	qglScaled                   = dllScaled						= QGL_GetProcAddress("glScaled");
	qglScalef                   = dllScalef						= QGL_GetProcAddress("glScalef");
	qglScissor                  = dllScissor					= QGL_GetProcAddress("glScissor");
	qglSelectBuffer             = dllSelectBuffer				= QGL_GetProcAddress("glSelectBuffer");
	qglShadeModel               = dllShadeModel					= QGL_GetProcAddress("glShadeModel");
	qglStencilFunc              = dllStencilFunc				= QGL_GetProcAddress("glStencilFunc");
	qglStencilMask              = dllStencilMask				= QGL_GetProcAddress("glStencilMask");
	qglStencilOp                = dllStencilOp					= QGL_GetProcAddress("glStencilOp");
	qglTexCoord1d               = dllTexCoord1d					= QGL_GetProcAddress("glTexCoord1d");
	qglTexCoord1dv              = dllTexCoord1dv				= QGL_GetProcAddress("glTexCoord1dv");
	qglTexCoord1f               = dllTexCoord1f					= QGL_GetProcAddress("glTexCoord1f");
	qglTexCoord1fv              = dllTexCoord1fv				= QGL_GetProcAddress("glTexCoord1fv");
	qglTexCoord1i               = dllTexCoord1i					= QGL_GetProcAddress("glTexCoord1i");
	qglTexCoord1iv              = dllTexCoord1iv				= QGL_GetProcAddress("glTexCoord1iv");
	qglTexCoord1s               = dllTexCoord1s					= QGL_GetProcAddress("glTexCoord1s");
	qglTexCoord1sv              = dllTexCoord1sv				= QGL_GetProcAddress("glTexCoord1sv");
	qglTexCoord2d               = dllTexCoord2d					= QGL_GetProcAddress("glTexCoord2d");
	qglTexCoord2dv              = dllTexCoord2dv				= QGL_GetProcAddress("glTexCoord2dv");
	qglTexCoord2f               = dllTexCoord2f					= QGL_GetProcAddress("glTexCoord2f");
	qglTexCoord2fv              = dllTexCoord2fv				= QGL_GetProcAddress("glTexCoord2fv");
	qglTexCoord2i               = dllTexCoord2i					= QGL_GetProcAddress("glTexCoord2i");
	qglTexCoord2iv              = dllTexCoord2iv				= QGL_GetProcAddress("glTexCoord2iv");
	qglTexCoord2s               = dllTexCoord2s					= QGL_GetProcAddress("glTexCoord2s");
	qglTexCoord2sv              = dllTexCoord2sv				= QGL_GetProcAddress("glTexCoord2sv");
	qglTexCoord3d               = dllTexCoord3d					= QGL_GetProcAddress("glTexCoord3d");
	qglTexCoord3dv              = dllTexCoord3dv				= QGL_GetProcAddress("glTexCoord3dv");
	qglTexCoord3f               = dllTexCoord3f					= QGL_GetProcAddress("glTexCoord3f");
	qglTexCoord3fv              = dllTexCoord3fv				= QGL_GetProcAddress("glTexCoord3fv");
	qglTexCoord3i               = dllTexCoord3i					= QGL_GetProcAddress("glTexCoord3i");
	qglTexCoord3iv              = dllTexCoord3iv				= QGL_GetProcAddress("glTexCoord3iv");
	qglTexCoord3s               = dllTexCoord3s					= QGL_GetProcAddress("glTexCoord3s");
	qglTexCoord3sv              = dllTexCoord3sv				= QGL_GetProcAddress("glTexCoord3sv");
	qglTexCoord4d               = dllTexCoord4d					= QGL_GetProcAddress("glTexCoord4d");
	qglTexCoord4dv              = dllTexCoord4dv				= QGL_GetProcAddress("glTexCoord4dv");
	qglTexCoord4f               = dllTexCoord4f					= QGL_GetProcAddress("glTexCoord4f");
	qglTexCoord4fv              = dllTexCoord4fv				= QGL_GetProcAddress("glTexCoord4fv");
	qglTexCoord4i               = dllTexCoord4i					= QGL_GetProcAddress("glTexCoord4i");
	qglTexCoord4iv              = dllTexCoord4iv				= QGL_GetProcAddress("glTexCoord4iv");
	qglTexCoord4s               = dllTexCoord4s					= QGL_GetProcAddress("glTexCoord4s");
	qglTexCoord4sv              = dllTexCoord4sv				= QGL_GetProcAddress("glTexCoord4sv");
	qglTexCoordPointer          = dllTexCoordPointer			= QGL_GetProcAddress("glTexCoordPointer");
	qglTexEnvf                  = dllTexEnvf					= QGL_GetProcAddress("glTexEnvf");
	qglTexEnvfv                 = dllTexEnvfv					= QGL_GetProcAddress("glTexEnvfv");
	qglTexEnvi                  = dllTexEnvi					= QGL_GetProcAddress("glTexEnvi");
	qglTexEnviv                 = dllTexEnviv					= QGL_GetProcAddress("glTexEnviv");
	qglTexGend                  = dllTexGend					= QGL_GetProcAddress("glTexGend");
	qglTexGendv                 = dllTexGendv					= QGL_GetProcAddress("glTexGendv");
	qglTexGenf                  = dllTexGenf					= QGL_GetProcAddress("glTexGenf");
	qglTexGenfv                 = dllTexGenfv					= QGL_GetProcAddress("glTexGenfv");
	qglTexGeni                  = dllTexGeni					= QGL_GetProcAddress("glTexGeni");
	qglTexGeniv                 = dllTexGeniv					= QGL_GetProcAddress("glTexGeniv");
	qglTexImage1D               = dllTexImage1D					= QGL_GetProcAddress("glTexImage1D");
	qglTexImage2D               = dllTexImage2D					= QGL_GetProcAddress("glTexImage2D");
	qglTexParameterf            = dllTexParameterf				= QGL_GetProcAddress("glTexParameterf");
	qglTexParameterfv           = dllTexParameterfv				= QGL_GetProcAddress("glTexParameterfv");
	qglTexParameteri            = dllTexParameteri				= QGL_GetProcAddress("glTexParameteri");
	qglTexParameteriv           = dllTexParameteriv				= QGL_GetProcAddress("glTexParameteriv");
	qglTexSubImage1D            = dllTexSubImage1D				= QGL_GetProcAddress("glTexSubImage1D");
	qglTexSubImage2D            = dllTexSubImage2D				= QGL_GetProcAddress("glTexSubImage2D");
	qglTranslated               = dllTranslated					= QGL_GetProcAddress("glTranslated");
	qglTranslatef               = dllTranslatef					= QGL_GetProcAddress("glTranslatef");
	qglVertex2d                 = dllVertex2d					= QGL_GetProcAddress("glVertex2d");
	qglVertex2dv                = dllVertex2dv					= QGL_GetProcAddress("glVertex2dv");
	qglVertex2f                 = dllVertex2f					= QGL_GetProcAddress("glVertex2f");
	qglVertex2fv                = dllVertex2fv					= QGL_GetProcAddress("glVertex2fv");
	qglVertex2i                 = dllVertex2i					= QGL_GetProcAddress("glVertex2i");
	qglVertex2iv                = dllVertex2iv					= QGL_GetProcAddress("glVertex2iv");
	qglVertex2s                 = dllVertex2s					= QGL_GetProcAddress("glVertex2s");
	qglVertex2sv                = dllVertex2sv					= QGL_GetProcAddress("glVertex2sv");
	qglVertex3d                 = dllVertex3d					= QGL_GetProcAddress("glVertex3d");
	qglVertex3dv                = dllVertex3dv					= QGL_GetProcAddress("glVertex3dv");
	qglVertex3f                 = dllVertex3f					= QGL_GetProcAddress("glVertex3f");
	qglVertex3fv                = dllVertex3fv					= QGL_GetProcAddress("glVertex3fv");
	qglVertex3i                 = dllVertex3i					= QGL_GetProcAddress("glVertex3i");
	qglVertex3iv                = dllVertex3iv					= QGL_GetProcAddress("glVertex3iv");
	qglVertex3s                 = dllVertex3s					= QGL_GetProcAddress("glVertex3s");
	qglVertex3sv                = dllVertex3sv					= QGL_GetProcAddress("glVertex3sv");
	qglVertex4d                 = dllVertex4d					= QGL_GetProcAddress("glVertex4d");
	qglVertex4dv                = dllVertex4dv					= QGL_GetProcAddress("glVertex4dv");
	qglVertex4f                 = dllVertex4f					= QGL_GetProcAddress("glVertex4f");
	qglVertex4fv                = dllVertex4fv					= QGL_GetProcAddress("glVertex4fv");
	qglVertex4i                 = dllVertex4i					= QGL_GetProcAddress("glVertex4i");
	qglVertex4iv                = dllVertex4iv					= QGL_GetProcAddress("glVertex4iv");
	qglVertex4s                 = dllVertex4s					= QGL_GetProcAddress("glVertex4s");
	qglVertex4sv                = dllVertex4sv					= QGL_GetProcAddress("glVertex4sv");
	qglVertexPointer            = dllVertexPointer				= QGL_GetProcAddress("glVertexPointer");
	qglViewport                 = dllViewport					= QGL_GetProcAddress("glViewport");

	qglActiveTextureARB					= dllActiveTextureARB				= NULL;
	qglClientActiveTextureARB			= dllClientActiveTextureARB			= NULL;

	qglBindBufferARB					= dllBindBufferARB					= NULL;
	qglDeleteBuffersARB					= dllDeleteBuffersARB				= NULL;
	qglGenBuffersARB					= dllGenBuffersARB					= NULL;
	qglBufferDataARB					= dllBufferDataARB					= NULL;
	qglBufferSubDataARB					= dllBufferSubDataARB				= NULL;
	qglMapBufferARB						= dllMapBufferARB					= NULL;
	qglUnmapBufferARB					= dllUnmapBufferARB					= NULL;

	qglVertexAttribPointerARB			= dllVertexAttribPointerARB			= NULL;
	qglEnableVertexAttribArrayARB		= dllEnableVertexAttribArrayARB		= NULL;
	qglDisableVertexAttribArrayARB		= dllDisableVertexAttribArrayARB	= NULL;
	qglBindProgramARB					= dllBindProgramARB					= NULL;
	qglDeleteProgramsARB				= dllDeleteProgramsARB				= NULL;
	qglGenProgramsARB					= dllGenProgramsARB					= NULL;
	qglProgramStringARB					= dllProgramStringARB				= NULL;
	qglProgramEnvParameter4fARB			= dllProgramEnvParameter4fARB		= NULL;
	qglProgramEnvParameter4fvARB		= dllProgramEnvParameter4fvARB		= NULL;
	qglProgramLocalParameter4fARB		= dllProgramLocalParameter4fARB		= NULL;
	qglProgramLocalParameter4fvARB		= dllProgramLocalParameter4fvARB	= NULL;
	qglGetProgramivARB					= dllGetProgramivARB				= NULL;

	qglDrawRangeElementsEXT				= dllDrawRangeElementsEXT			= NULL;

	qglActiveStencilFaceEXT				= dllActiveStencilFaceEXT			= NULL;

	qglDepthBoundsEXT					= dllDepthBoundsEXT					= NULL;

	qwglSwapIntervalEXT					= dllSwapIntervalEXT				= NULL;

	qglCombinerParameterfvNV			= dllCombinerParameterfvNV			= NULL;
	qglCombinerParameterfNV				= dllCombinerParameterfNV			= NULL;
	qglCombinerParameterivNV			= dllCombinerParameterivNV			= NULL;
	qglCombinerParameteriNV				= dllCombinerParameteriNV			= NULL;
	qglCombinerInputNV					= dllCombinerInputNV				= NULL;
	qglCombinerOutputNV					= dllCombinerOutputNV				= NULL;
	qglFinalCombinerInputNV				= dllFinalCombinerInputNV			= NULL;

	qglGenFragmentShadersATI			= dllGenFragmentShadersATI			= NULL;
	qglBindFragmentShaderATI			= dllBindFragmentShaderATI			= NULL;
	qglDeleteFragmentShaderATI			= dllDeleteFragmentShaderATI		= NULL;
	qglBeginFragmentShaderATI			= dllBeginFragmentShaderATI			= NULL;
	qglEndFragmentShaderATI				= dllEndFragmentShaderATI			= NULL;
	qglPassTexCoordATI					= dllPassTexCoordATI				= NULL;
	qglSampleMapATI						= dllSampleMapATI					= NULL;
	qglColorFragmentOp1ATI				= dllColorFragmentOp1ATI			= NULL;
	qglColorFragmentOp2ATI				= dllColorFragmentOp2ATI			= NULL;
	qglColorFragmentOp3ATI				= dllColorFragmentOp3ATI			= NULL;
	qglAlphaFragmentOp1ATI				= dllAlphaFragmentOp1ATI			= NULL;
	qglAlphaFragmentOp2ATI				= dllAlphaFragmentOp2ATI			= NULL;
	qglAlphaFragmentOp3ATI				= dllAlphaFragmentOp3ATI			= NULL;
	qglSetFragmentShaderConstantATI		= dllSetFragmentShaderConstantATI	= NULL;

	qglStencilOpSeparateATI				= dllStencilOpSeparateATI			= NULL;
	qglStencilFuncSeparateATI			= dllStencilFuncSeparateATI			= NULL;

	return true;
}

/*
 =================
 QGL_EnableLogging
 =================
*/
void QGL_EnableLogging (qboolean enable){

	time_t		clock;
	struct tm	*curTime;

	// Extensions are initialized later, so we need to set the pointers
	if (!qglExtensionPointers){
		qglExtensionPointers = true;

		dllActiveTextureARB					= qglActiveTextureARB;
		dllClientActiveTextureARB			= qglClientActiveTextureARB;

		dllBindBufferARB					= qglBindBufferARB;
		dllDeleteBuffersARB					= qglDeleteBuffersARB;
		dllGenBuffersARB					= qglGenBuffersARB;
		dllBufferDataARB					= qglBufferDataARB;
		dllBufferSubDataARB					= qglBufferSubDataARB;
		dllMapBufferARB						= qglMapBufferARB;
		dllUnmapBufferARB					= qglUnmapBufferARB;

		dllVertexAttribPointerARB			= qglVertexAttribPointerARB;
		dllEnableVertexAttribArrayARB		= qglEnableVertexAttribArrayARB;
		dllDisableVertexAttribArrayARB		= qglDisableVertexAttribArrayARB;
		dllBindProgramARB					= qglBindProgramARB;
		dllDeleteProgramsARB				= qglDeleteProgramsARB;
		dllGenProgramsARB					= qglGenProgramsARB;
		dllProgramStringARB					= qglProgramStringARB;
		dllProgramEnvParameter4fARB			= qglProgramEnvParameter4fARB;
		dllProgramEnvParameter4fvARB		= qglProgramEnvParameter4fvARB;
		dllProgramLocalParameter4fARB		= qglProgramLocalParameter4fARB;
		dllProgramLocalParameter4fvARB		= qglProgramLocalParameter4fvARB;
		dllGetProgramivARB					= qglGetProgramivARB;

		dllDrawRangeElementsEXT				= qglDrawRangeElementsEXT;

		dllActiveStencilFaceEXT				= qglActiveStencilFaceEXT;

		dllDepthBoundsEXT					= qglDepthBoundsEXT;

		dllSwapIntervalEXT					= qwglSwapIntervalEXT;

		dllCombinerParameterfvNV			= qglCombinerParameterfvNV;
		dllCombinerParameterfNV				= qglCombinerParameterfNV;
		dllCombinerParameterivNV			= qglCombinerParameterivNV;
		dllCombinerParameteriNV				= qglCombinerParameteriNV;
		dllCombinerInputNV					= qglCombinerInputNV;
		dllCombinerOutputNV					= qglCombinerOutputNV;
		dllFinalCombinerInputNV				= qglFinalCombinerInputNV;

		dllGenFragmentShadersATI			= qglGenFragmentShadersATI;
		dllBindFragmentShaderATI			= qglBindFragmentShaderATI;
		dllDeleteFragmentShaderATI			= qglDeleteFragmentShaderATI;
		dllBeginFragmentShaderATI			= qglBeginFragmentShaderATI;
		dllEndFragmentShaderATI				= qglEndFragmentShaderATI;
		dllPassTexCoordATI					= qglPassTexCoordATI;
		dllSampleMapATI						= qglSampleMapATI;
		dllColorFragmentOp1ATI				= qglColorFragmentOp1ATI;
		dllColorFragmentOp2ATI				= qglColorFragmentOp2ATI;
		dllColorFragmentOp3ATI				= qglColorFragmentOp3ATI;
		dllAlphaFragmentOp1ATI				= qglAlphaFragmentOp1ATI;
		dllAlphaFragmentOp2ATI				= qglAlphaFragmentOp2ATI;
		dllAlphaFragmentOp3ATI				= qglAlphaFragmentOp3ATI;
		dllSetFragmentShaderConstantATI		= qglSetFragmentShaderConstantATI;

		dllStencilOpSeparateATI				= qglStencilOpSeparateATI;
		dllStencilFuncSeparateATI			= qglStencilFuncSeparateATI;
	}

	if (enable){
		if (glwState.logFile)
			return;

#ifdef SECURE
		glwState.logFile = fopen_s(glwState.logFile, "gl.log", "at");
#else
		glwState.logFile = fopen("gl.log", "at");
#endif
		if (!glwState.logFile)
			return;

		time(&clock);
#ifdef SECURE
		curTime = localtime_s(&curTime, &clock);
#else
		curTime = localtime(&clock);
#endif
		fprintf(glwState.logFile, "%s %s (%s)\n", Q2E_VERSION, BUILDSTRING, __DATE__);
#ifdef SECURE
		fprintf(glwState.logFile, "Log file opened on %s\n\n", asctime_s(curTime, sizeof(curTime), clock));
#else
		fprintf(glwState.logFile, "Log file opened on %s\n\n", asctime(curTime));
#endif

		qglAccum                     = logAccum;
		qglAlphaFunc                 = logAlphaFunc;
		qglAreTexturesResident       = logAreTexturesResident;
		qglArrayElement              = logArrayElement;
		qglBegin                     = logBegin;
		qglBindTexture               = logBindTexture;
		qglBitmap                    = logBitmap;
		qglBlendFunc                 = logBlendFunc;
		qglCallList                  = logCallList;
		qglCallLists                 = logCallLists;
		qglClear                     = logClear;
		qglClearAccum                = logClearAccum;
		qglClearColor                = logClearColor;
		qglClearDepth                = logClearDepth;
		qglClearIndex                = logClearIndex;
		qglClearStencil              = logClearStencil;
		qglClipPlane                 = logClipPlane;
		qglColor3b                   = logColor3b;
		qglColor3bv                  = logColor3bv;
		qglColor3d                   = logColor3d;
		qglColor3dv                  = logColor3dv;
		qglColor3f                   = logColor3f;
		qglColor3fv                  = logColor3fv;
		qglColor3i                   = logColor3i;
		qglColor3iv                  = logColor3iv;
		qglColor3s                   = logColor3s;
		qglColor3sv                  = logColor3sv;
		qglColor3ub                  = logColor3ub;
		qglColor3ubv                 = logColor3ubv;
		qglColor3ui                  = logColor3ui;
		qglColor3uiv                 = logColor3uiv;
		qglColor3us                  = logColor3us;
		qglColor3usv                 = logColor3usv;
		qglColor4b                   = logColor4b;
		qglColor4bv                  = logColor4bv;
		qglColor4d                   = logColor4d;
		qglColor4dv                  = logColor4dv;
		qglColor4f                   = logColor4f;
		qglColor4fv                  = logColor4fv;
		qglColor4i                   = logColor4i;
		qglColor4iv                  = logColor4iv;
		qglColor4s                   = logColor4s;
		qglColor4sv                  = logColor4sv;
		qglColor4ub                  = logColor4ub;
		qglColor4ubv                 = logColor4ubv;
		qglColor4ui                  = logColor4ui;
		qglColor4uiv                 = logColor4uiv;
		qglColor4us                  = logColor4us;
		qglColor4usv                 = logColor4usv;
		qglColorMask                 = logColorMask;
		qglColorMaterial             = logColorMaterial;
		qglColorPointer              = logColorPointer;
		qglCopyPixels                = logCopyPixels;
		qglCopyTexImage1D            = logCopyTexImage1D;
		qglCopyTexImage2D            = logCopyTexImage2D;
		qglCopyTexSubImage1D         = logCopyTexSubImage1D;
		qglCopyTexSubImage2D         = logCopyTexSubImage2D;
		qglCullFace                  = logCullFace;
		qglDeleteLists               = logDeleteLists;
		qglDeleteTextures            = logDeleteTextures;
		qglDepthFunc                 = logDepthFunc;
		qglDepthMask                 = logDepthMask;
		qglDepthRange                = logDepthRange;
		qglDisable                   = logDisable;
		qglDisableClientState        = logDisableClientState;
		qglDrawArrays                = logDrawArrays;
		qglDrawBuffer                = logDrawBuffer;
		qglDrawElements              = logDrawElements;
		qglDrawPixels                = logDrawPixels;
		qglEdgeFlag                  = logEdgeFlag;
		qglEdgeFlagPointer           = logEdgeFlagPointer;
		qglEdgeFlagv                 = logEdgeFlagv;
		qglEnable                    = logEnable;
		qglEnableClientState         = logEnableClientState;
		qglEnd                       = logEnd;
		qglEndList                   = logEndList;
		qglEvalCoord1d				 = logEvalCoord1d;
		qglEvalCoord1dv              = logEvalCoord1dv;
		qglEvalCoord1f               = logEvalCoord1f;
		qglEvalCoord1fv              = logEvalCoord1fv;
		qglEvalCoord2d               = logEvalCoord2d;
		qglEvalCoord2dv              = logEvalCoord2dv;
		qglEvalCoord2f               = logEvalCoord2f;
		qglEvalCoord2fv              = logEvalCoord2fv;
		qglEvalMesh1                 = logEvalMesh1;
		qglEvalMesh2                 = logEvalMesh2;
		qglEvalPoint1                = logEvalPoint1;
		qglEvalPoint2                = logEvalPoint2;
		qglFeedbackBuffer            = logFeedbackBuffer;
		qglFinish                    = logFinish;
		qglFlush                     = logFlush;
		qglFogf                      = logFogf;
		qglFogfv                     = logFogfv;
		qglFogi                      = logFogi;
		qglFogiv                     = logFogiv;
		qglFrontFace                 = logFrontFace;
		qglFrustum                   = logFrustum;
		qglGenLists                  = logGenLists;
		qglGenTextures               = logGenTextures;
		qglGetBooleanv               = logGetBooleanv;
		qglGetClipPlane              = logGetClipPlane;
		qglGetDoublev                = logGetDoublev;
		qglGetError                  = logGetError;
		qglGetFloatv                 = logGetFloatv;
		qglGetIntegerv               = logGetIntegerv;
		qglGetLightfv                = logGetLightfv;
		qglGetLightiv                = logGetLightiv;
		qglGetMapdv                  = logGetMapdv;
		qglGetMapfv                  = logGetMapfv;
		qglGetMapiv                  = logGetMapiv;
		qglGetMaterialfv             = logGetMaterialfv;
		qglGetMaterialiv             = logGetMaterialiv;
		qglGetPixelMapfv             = logGetPixelMapfv;
		qglGetPixelMapuiv            = logGetPixelMapuiv;
		qglGetPixelMapusv            = logGetPixelMapusv;
		qglGetPointerv               = logGetPointerv;
		qglGetPolygonStipple         = logGetPolygonStipple;
		qglGetString                 = logGetString;
		qglGetTexEnvfv               = logGetTexEnvfv;
		qglGetTexEnviv               = logGetTexEnviv;
		qglGetTexGendv               = logGetTexGendv;
		qglGetTexGenfv               = logGetTexGenfv;
		qglGetTexGeniv               = logGetTexGeniv;
		qglGetTexImage               = logGetTexImage;
		qglGetTexLevelParameterfv    = logGetTexLevelParameterfv;
		qglGetTexLevelParameteriv    = logGetTexLevelParameteriv;
		qglGetTexParameterfv         = logGetTexParameterfv;
		qglGetTexParameteriv         = logGetTexParameteriv;
		qglHint                      = logHint;
		qglIndexMask                 = logIndexMask;
		qglIndexPointer              = logIndexPointer;
		qglIndexd                    = logIndexd;
		qglIndexdv                   = logIndexdv;
		qglIndexf                    = logIndexf;
		qglIndexfv                   = logIndexfv;
		qglIndexi                    = logIndexi;
		qglIndexiv                   = logIndexiv;
		qglIndexs                    = logIndexs;
		qglIndexsv                   = logIndexsv;
		qglIndexub                   = logIndexub;
		qglIndexubv                  = logIndexubv;
		qglInitNames                 = logInitNames;
		qglInterleavedArrays         = logInterleavedArrays;
		qglIsEnabled                 = logIsEnabled;
		qglIsList                    = logIsList;
		qglIsTexture                 = logIsTexture;
		qglLightModelf               = logLightModelf;
		qglLightModelfv              = logLightModelfv;
		qglLightModeli               = logLightModeli;
		qglLightModeliv              = logLightModeliv;
		qglLightf                    = logLightf;
		qglLightfv                   = logLightfv;
		qglLighti                    = logLighti;
		qglLightiv                   = logLightiv;
		qglLineStipple               = logLineStipple;
		qglLineWidth                 = logLineWidth;
		qglListBase                  = logListBase;
		qglLoadIdentity              = logLoadIdentity;
		qglLoadMatrixd               = logLoadMatrixd;
		qglLoadMatrixf               = logLoadMatrixf;
		qglLoadName                  = logLoadName;
		qglLogicOp                   = logLogicOp;
		qglMap1d                     = logMap1d;
		qglMap1f                     = logMap1f;
		qglMap2d                     = logMap2d;
		qglMap2f                     = logMap2f;
		qglMapGrid1d                 = logMapGrid1d;
		qglMapGrid1f                 = logMapGrid1f;
		qglMapGrid2d                 = logMapGrid2d;
		qglMapGrid2f                 = logMapGrid2f;
		qglMaterialf                 = logMaterialf;
		qglMaterialfv                = logMaterialfv;
		qglMateriali                 = logMateriali;
		qglMaterialiv                = logMaterialiv;
		qglMatrixMode                = logMatrixMode;
		qglMultMatrixd               = logMultMatrixd;
		qglMultMatrixf               = logMultMatrixf;
		qglNewList                   = logNewList;
		qglNormal3b                  = logNormal3b;
		qglNormal3bv                 = logNormal3bv;
		qglNormal3d                  = logNormal3d;
		qglNormal3dv                 = logNormal3dv;
		qglNormal3f                  = logNormal3f;
		qglNormal3fv                 = logNormal3fv;
		qglNormal3i                  = logNormal3i;
		qglNormal3iv                 = logNormal3iv;
		qglNormal3s                  = logNormal3s;
		qglNormal3sv                 = logNormal3sv;
		qglNormalPointer             = logNormalPointer;
		qglOrtho                     = logOrtho;
		qglPassThrough               = logPassThrough;
		qglPixelMapfv                = logPixelMapfv;
		qglPixelMapuiv               = logPixelMapuiv;
		qglPixelMapusv               = logPixelMapusv;
		qglPixelStoref               = logPixelStoref;
		qglPixelStorei               = logPixelStorei;
		qglPixelTransferf            = logPixelTransferf;
		qglPixelTransferi            = logPixelTransferi;
		qglPixelZoom                 = logPixelZoom;
		qglPointSize                 = logPointSize;
		qglPolygonMode               = logPolygonMode;
		qglPolygonOffset             = logPolygonOffset;
		qglPolygonStipple            = logPolygonStipple;
		qglPopAttrib                 = logPopAttrib;
		qglPopClientAttrib           = logPopClientAttrib;
		qglPopMatrix                 = logPopMatrix;
		qglPopName                   = logPopName;
		qglPrioritizeTextures        = logPrioritizeTextures;
		qglPushAttrib                = logPushAttrib;
		qglPushClientAttrib          = logPushClientAttrib;
		qglPushMatrix                = logPushMatrix;
		qglPushName                  = logPushName;
		qglRasterPos2d               = logRasterPos2d;
		qglRasterPos2dv              = logRasterPos2dv;
		qglRasterPos2f               = logRasterPos2f;
		qglRasterPos2fv              = logRasterPos2fv;
		qglRasterPos2i               = logRasterPos2i;
		qglRasterPos2iv              = logRasterPos2iv;
		qglRasterPos2s               = logRasterPos2s;
		qglRasterPos2sv              = logRasterPos2sv;
		qglRasterPos3d               = logRasterPos3d;
		qglRasterPos3dv              = logRasterPos3dv;
		qglRasterPos3f               = logRasterPos3f;
		qglRasterPos3fv              = logRasterPos3fv;
		qglRasterPos3i               = logRasterPos3i;
		qglRasterPos3iv              = logRasterPos3iv;
		qglRasterPos3s               = logRasterPos3s;
		qglRasterPos3sv              = logRasterPos3sv;
		qglRasterPos4d               = logRasterPos4d;
		qglRasterPos4dv              = logRasterPos4dv;
		qglRasterPos4f               = logRasterPos4f;
		qglRasterPos4fv              = logRasterPos4fv;
		qglRasterPos4i               = logRasterPos4i;
		qglRasterPos4iv              = logRasterPos4iv;
		qglRasterPos4s               = logRasterPos4s;
		qglRasterPos4sv              = logRasterPos4sv;
		qglReadBuffer                = logReadBuffer;
		qglReadPixels                = logReadPixels;
		qglRectd                     = logRectd;
		qglRectdv                    = logRectdv;
		qglRectf                     = logRectf;
		qglRectfv                    = logRectfv;
		qglRecti                     = logRecti;
		qglRectiv                    = logRectiv;
		qglRects                     = logRects;
		qglRectsv                    = logRectsv;
		qglRenderMode                = logRenderMode;
		qglRotated                   = logRotated;
		qglRotatef                   = logRotatef;
		qglScaled                    = logScaled;
		qglScalef                    = logScalef;
		qglScissor                   = logScissor;
		qglSelectBuffer              = logSelectBuffer;
		qglShadeModel                = logShadeModel;
		qglStencilFunc               = logStencilFunc;
		qglStencilMask               = logStencilMask;
		qglStencilOp                 = logStencilOp;
		qglTexCoord1d                = logTexCoord1d;
		qglTexCoord1dv               = logTexCoord1dv;
		qglTexCoord1f                = logTexCoord1f;
		qglTexCoord1fv               = logTexCoord1fv;
		qglTexCoord1i                = logTexCoord1i;
		qglTexCoord1iv               = logTexCoord1iv;
		qglTexCoord1s                = logTexCoord1s;
		qglTexCoord1sv               = logTexCoord1sv;
		qglTexCoord2d                = logTexCoord2d;
		qglTexCoord2dv               = logTexCoord2dv;
		qglTexCoord2f                = logTexCoord2f;
		qglTexCoord2fv               = logTexCoord2fv;
		qglTexCoord2i                = logTexCoord2i;
		qglTexCoord2iv               = logTexCoord2iv;
		qglTexCoord2s                = logTexCoord2s;
		qglTexCoord2sv               = logTexCoord2sv;
		qglTexCoord3d                = logTexCoord3d;
		qglTexCoord3dv               = logTexCoord3dv;
		qglTexCoord3f                = logTexCoord3f;
		qglTexCoord3fv               = logTexCoord3fv;
		qglTexCoord3i                = logTexCoord3i;
		qglTexCoord3iv               = logTexCoord3iv;
		qglTexCoord3s                = logTexCoord3s;
		qglTexCoord3sv               = logTexCoord3sv;
		qglTexCoord4d                = logTexCoord4d;
		qglTexCoord4dv               = logTexCoord4dv;
		qglTexCoord4f                = logTexCoord4f;
		qglTexCoord4fv               = logTexCoord4fv;
		qglTexCoord4i                = logTexCoord4i;
		qglTexCoord4iv               = logTexCoord4iv;
		qglTexCoord4s                = logTexCoord4s;
		qglTexCoord4sv               = logTexCoord4sv;
		qglTexCoordPointer           = logTexCoordPointer;
		qglTexEnvf                   = logTexEnvf;
		qglTexEnvfv                  = logTexEnvfv;
		qglTexEnvi                   = logTexEnvi;
		qglTexEnviv                  = logTexEnviv;
		qglTexGend                   = logTexGend;
		qglTexGendv                  = logTexGendv;
		qglTexGenf                   = logTexGenf;
		qglTexGenfv                  = logTexGenfv;
		qglTexGeni                   = logTexGeni;
		qglTexGeniv                  = logTexGeniv;
		qglTexImage1D                = logTexImage1D;
		qglTexImage2D                = logTexImage2D;
		qglTexParameterf             = logTexParameterf;
		qglTexParameterfv            = logTexParameterfv;
		qglTexParameteri             = logTexParameteri;
		qglTexParameteriv            = logTexParameteriv;
		qglTexSubImage1D             = logTexSubImage1D;
		qglTexSubImage2D             = logTexSubImage2D;
		qglTranslated                = logTranslated;
		qglTranslatef                = logTranslatef;
		qglVertex2d                  = logVertex2d;
		qglVertex2dv                 = logVertex2dv;
		qglVertex2f                  = logVertex2f;
		qglVertex2fv                 = logVertex2fv;
		qglVertex2i                  = logVertex2i;
		qglVertex2iv                 = logVertex2iv;
		qglVertex2s                  = logVertex2s;
		qglVertex2sv                 = logVertex2sv;
		qglVertex3d                  = logVertex3d;
		qglVertex3dv                 = logVertex3dv;
		qglVertex3f                  = logVertex3f;
		qglVertex3fv                 = logVertex3fv;
		qglVertex3i                  = logVertex3i;
		qglVertex3iv                 = logVertex3iv;
		qglVertex3s                  = logVertex3s;
		qglVertex3sv                 = logVertex3sv;
		qglVertex4d                  = logVertex4d;
		qglVertex4dv                 = logVertex4dv;
		qglVertex4f                  = logVertex4f;
		qglVertex4fv                 = logVertex4fv;
		qglVertex4i                  = logVertex4i;
		qglVertex4iv                 = logVertex4iv;
		qglVertex4s                  = logVertex4s;
		qglVertex4sv                 = logVertex4sv;
		qglVertexPointer             = logVertexPointer;
		qglViewport                  = logViewport;

		qglActiveTextureARB					= logActiveTextureARB;
		qglClientActiveTextureARB			= logClientActiveTextureARB;

		qglBindBufferARB					= logBindBufferARB;
		qglDeleteBuffersARB					= logDeleteBuffersARB;
		qglGenBuffersARB					= logGenBuffersARB;
		qglBufferDataARB					= logBufferDataARB;
		qglBufferSubDataARB					= logBufferSubDataARB;
		qglMapBufferARB						= logMapBufferARB;
		qglUnmapBufferARB					= logUnmapBufferARB;

		qglVertexAttribPointerARB			= logVertexAttribPointerARB;
		qglEnableVertexAttribArrayARB		= logEnableVertexAttribArrayARB;
		qglDisableVertexAttribArrayARB		= logDisableVertexAttribArrayARB;
		qglBindProgramARB					= logBindProgramARB;
		qglDeleteProgramsARB				= logDeleteProgramsARB;
		qglGenProgramsARB					= logGenProgramsARB;
		qglProgramStringARB					= logProgramStringARB;
		qglProgramEnvParameter4fARB			= logProgramEnvParameter4fARB;
		qglProgramEnvParameter4fvARB		= logProgramEnvParameter4fvARB;
		qglProgramLocalParameter4fARB		= logProgramLocalParameter4fARB;
		qglProgramLocalParameter4fvARB		= logProgramLocalParameter4fvARB;
		qglGetProgramivARB					= logGetProgramivARB;

		qglDrawRangeElementsEXT				= logDrawRangeElementsEXT;

		qglActiveStencilFaceEXT				= logActiveStencilFaceEXT;

		qglDepthBoundsEXT					= logDepthBoundsEXT;

		qwglSwapIntervalEXT					= logSwapIntervalEXT;

		qglCombinerParameterfvNV			= logCombinerParameterfvNV;
		qglCombinerParameterfNV				= logCombinerParameterfNV;
		qglCombinerParameterivNV			= logCombinerParameterivNV;
		qglCombinerParameteriNV				= logCombinerParameteriNV;
		qglCombinerInputNV					= logCombinerInputNV;
		qglCombinerOutputNV					= logCombinerOutputNV;
		qglFinalCombinerInputNV				= logFinalCombinerInputNV;

		qglGenFragmentShadersATI			= logGenFragmentShadersATI;
		qglBindFragmentShaderATI			= logBindFragmentShaderATI;
		qglDeleteFragmentShaderATI			= logDeleteFragmentShaderATI;
		qglBeginFragmentShaderATI			= logBeginFragmentShaderATI;
		qglEndFragmentShaderATI				= logEndFragmentShaderATI;
		qglPassTexCoordATI					= logPassTexCoordATI;
		qglSampleMapATI						= logSampleMapATI;
		qglColorFragmentOp1ATI				= logColorFragmentOp1ATI;
		qglColorFragmentOp2ATI				= logColorFragmentOp2ATI;
		qglColorFragmentOp3ATI				= logColorFragmentOp3ATI;
		qglAlphaFragmentOp1ATI				= logAlphaFragmentOp1ATI;
		qglAlphaFragmentOp2ATI				= logAlphaFragmentOp2ATI;
		qglAlphaFragmentOp3ATI				= logAlphaFragmentOp3ATI;
		qglSetFragmentShaderConstantATI		= logSetFragmentShaderConstantATI;

		qglStencilOpSeparateATI				= logStencilOpSeparateATI;
		qglStencilFuncSeparateATI			= logStencilFuncSeparateATI;
	}
	else {
		if (!glwState.logFile)
			return;

		time(&clock);
#ifdef SECURE
		curTime = localtime_s(&curTime, &clock);
		fprintf(glwState.logFile, "\nLog file closed on %s\n\n", asctime_s(curTime, sizeof(curTime), clock));
#else
		curTime = localtime(&clock);
		fprintf(glwState.logFile, "\nLog file closed on %s\n\n", asctime(curTime));
#endif

		fclose(glwState.logFile);
		glwState.logFile = NULL;

		qglAccum                     = dllAccum;
		qglAlphaFunc                 = dllAlphaFunc;
		qglAreTexturesResident       = dllAreTexturesResident;
		qglArrayElement              = dllArrayElement;
		qglBegin                     = dllBegin;
		qglBindTexture               = dllBindTexture;
		qglBitmap                    = dllBitmap;
		qglBlendFunc                 = dllBlendFunc;
		qglCallList                  = dllCallList;
		qglCallLists                 = dllCallLists;
		qglClear                     = dllClear;
		qglClearAccum                = dllClearAccum;
		qglClearColor                = dllClearColor;
		qglClearDepth                = dllClearDepth;
		qglClearIndex                = dllClearIndex;
		qglClearStencil              = dllClearStencil;
		qglClipPlane                 = dllClipPlane;
		qglColor3b                   = dllColor3b;
		qglColor3bv                  = dllColor3bv;
		qglColor3d                   = dllColor3d;
		qglColor3dv                  = dllColor3dv;
		qglColor3f                   = dllColor3f;
		qglColor3fv                  = dllColor3fv;
		qglColor3i                   = dllColor3i;
		qglColor3iv                  = dllColor3iv;
		qglColor3s                   = dllColor3s;
		qglColor3sv                  = dllColor3sv;
		qglColor3ub                  = dllColor3ub;
		qglColor3ubv                 = dllColor3ubv;
		qglColor3ui                  = dllColor3ui;
		qglColor3uiv                 = dllColor3uiv;
		qglColor3us                  = dllColor3us;
		qglColor3usv                 = dllColor3usv;
		qglColor4b                   = dllColor4b;
		qglColor4bv                  = dllColor4bv;
		qglColor4d                   = dllColor4d;
		qglColor4dv                  = dllColor4dv;
		qglColor4f                   = dllColor4f;
		qglColor4fv                  = dllColor4fv;
		qglColor4i                   = dllColor4i;
		qglColor4iv                  = dllColor4iv;
		qglColor4s                   = dllColor4s;
		qglColor4sv                  = dllColor4sv;
		qglColor4ub                  = dllColor4ub;
		qglColor4ubv                 = dllColor4ubv;
		qglColor4ui                  = dllColor4ui;
		qglColor4uiv                 = dllColor4uiv;
		qglColor4us                  = dllColor4us;
		qglColor4usv                 = dllColor4usv;
		qglColorMask                 = dllColorMask;
		qglColorMaterial             = dllColorMaterial;
		qglColorPointer              = dllColorPointer;
		qglCopyPixels                = dllCopyPixels;
		qglCopyTexImage1D            = dllCopyTexImage1D;
		qglCopyTexImage2D            = dllCopyTexImage2D;
		qglCopyTexSubImage1D         = dllCopyTexSubImage1D;
		qglCopyTexSubImage2D         = dllCopyTexSubImage2D;
		qglCullFace                  = dllCullFace;
		qglDeleteLists               = dllDeleteLists;
		qglDeleteTextures            = dllDeleteTextures;
		qglDepthFunc                 = dllDepthFunc;
		qglDepthMask                 = dllDepthMask;
		qglDepthRange                = dllDepthRange;
		qglDisable                   = dllDisable;
		qglDisableClientState        = dllDisableClientState;
		qglDrawArrays                = dllDrawArrays;
		qglDrawBuffer                = dllDrawBuffer;
		qglDrawElements              = dllDrawElements;
		qglDrawPixels                = dllDrawPixels;
		qglEdgeFlag                  = dllEdgeFlag;
		qglEdgeFlagPointer           = dllEdgeFlagPointer;
		qglEdgeFlagv                 = dllEdgeFlagv;
		qglEnable                    = dllEnable;
		qglEnableClientState         = dllEnableClientState;
		qglEnd                       = dllEnd;
		qglEndList                   = dllEndList;
		qglEvalCoord1d				 = dllEvalCoord1d;
		qglEvalCoord1dv              = dllEvalCoord1dv;
		qglEvalCoord1f               = dllEvalCoord1f;
		qglEvalCoord1fv              = dllEvalCoord1fv;
		qglEvalCoord2d               = dllEvalCoord2d;
		qglEvalCoord2dv              = dllEvalCoord2dv;
		qglEvalCoord2f               = dllEvalCoord2f;
		qglEvalCoord2fv              = dllEvalCoord2fv;
		qglEvalMesh1                 = dllEvalMesh1;
		qglEvalMesh2                 = dllEvalMesh2;
		qglEvalPoint1                = dllEvalPoint1;
		qglEvalPoint2                = dllEvalPoint2;
		qglFeedbackBuffer            = dllFeedbackBuffer;
		qglFinish                    = dllFinish;
		qglFlush                     = dllFlush;
		qglFogf                      = dllFogf;
		qglFogfv                     = dllFogfv;
		qglFogi                      = dllFogi;
		qglFogiv                     = dllFogiv;
		qglFrontFace                 = dllFrontFace;
		qglFrustum                   = dllFrustum;
		qglGenLists                  = dllGenLists;
		qglGenTextures               = dllGenTextures;
		qglGetBooleanv               = dllGetBooleanv;
		qglGetClipPlane              = dllGetClipPlane;
		qglGetDoublev                = dllGetDoublev;
		qglGetError                  = dllGetError;
		qglGetFloatv                 = dllGetFloatv;
		qglGetIntegerv               = dllGetIntegerv;
		qglGetLightfv                = dllGetLightfv;
		qglGetLightiv                = dllGetLightiv;
		qglGetMapdv                  = dllGetMapdv;
		qglGetMapfv                  = dllGetMapfv;
		qglGetMapiv                  = dllGetMapiv;
		qglGetMaterialfv             = dllGetMaterialfv;
		qglGetMaterialiv             = dllGetMaterialiv;
		qglGetPixelMapfv             = dllGetPixelMapfv;
		qglGetPixelMapuiv            = dllGetPixelMapuiv;
		qglGetPixelMapusv            = dllGetPixelMapusv;
		qglGetPointerv               = dllGetPointerv;
		qglGetPolygonStipple         = dllGetPolygonStipple;
		qglGetString                 = dllGetString;
		qglGetTexEnvfv               = dllGetTexEnvfv;
		qglGetTexEnviv               = dllGetTexEnviv;
		qglGetTexGendv               = dllGetTexGendv;
		qglGetTexGenfv               = dllGetTexGenfv;
		qglGetTexGeniv               = dllGetTexGeniv;
		qglGetTexImage               = dllGetTexImage;
		qglGetTexLevelParameterfv    = dllGetTexLevelParameterfv;
		qglGetTexLevelParameteriv    = dllGetTexLevelParameteriv;
		qglGetTexParameterfv         = dllGetTexParameterfv;
		qglGetTexParameteriv         = dllGetTexParameteriv;
		qglHint                      = dllHint;
		qglIndexMask                 = dllIndexMask;
		qglIndexPointer              = dllIndexPointer;
		qglIndexd                    = dllIndexd;
		qglIndexdv                   = dllIndexdv;
		qglIndexf                    = dllIndexf;
		qglIndexfv                   = dllIndexfv;
		qglIndexi                    = dllIndexi;
		qglIndexiv                   = dllIndexiv;
		qglIndexs                    = dllIndexs;
		qglIndexsv                   = dllIndexsv;
		qglIndexub                   = dllIndexub;
		qglIndexubv                  = dllIndexubv;
		qglInitNames                 = dllInitNames;
		qglInterleavedArrays         = dllInterleavedArrays;
		qglIsEnabled                 = dllIsEnabled;
		qglIsList                    = dllIsList;
		qglIsTexture                 = dllIsTexture;
		qglLightModelf               = dllLightModelf;
		qglLightModelfv              = dllLightModelfv;
		qglLightModeli               = dllLightModeli;
		qglLightModeliv              = dllLightModeliv;
		qglLightf                    = dllLightf;
		qglLightfv                   = dllLightfv;
		qglLighti                    = dllLighti;
		qglLightiv                   = dllLightiv;
		qglLineStipple               = dllLineStipple;
		qglLineWidth                 = dllLineWidth;
		qglListBase                  = dllListBase;
		qglLoadIdentity              = dllLoadIdentity;
		qglLoadMatrixd               = dllLoadMatrixd;
		qglLoadMatrixf               = dllLoadMatrixf;
		qglLoadName                  = dllLoadName;
		qglLogicOp                   = dllLogicOp;
		qglMap1d                     = dllMap1d;
		qglMap1f                     = dllMap1f;
		qglMap2d                     = dllMap2d;
		qglMap2f                     = dllMap2f;
		qglMapGrid1d                 = dllMapGrid1d;
		qglMapGrid1f                 = dllMapGrid1f;
		qglMapGrid2d                 = dllMapGrid2d;
		qglMapGrid2f                 = dllMapGrid2f;
		qglMaterialf                 = dllMaterialf;
		qglMaterialfv                = dllMaterialfv;
		qglMateriali                 = dllMateriali;
		qglMaterialiv                = dllMaterialiv;
		qglMatrixMode                = dllMatrixMode;
		qglMultMatrixd               = dllMultMatrixd;
		qglMultMatrixf               = dllMultMatrixf;
		qglNewList                   = dllNewList;
		qglNormal3b                  = dllNormal3b;
		qglNormal3bv                 = dllNormal3bv;
		qglNormal3d                  = dllNormal3d;
		qglNormal3dv                 = dllNormal3dv;
		qglNormal3f                  = dllNormal3f;
		qglNormal3fv                 = dllNormal3fv;
		qglNormal3i                  = dllNormal3i;
		qglNormal3iv                 = dllNormal3iv;
		qglNormal3s                  = dllNormal3s;
		qglNormal3sv                 = dllNormal3sv;
		qglNormalPointer             = dllNormalPointer;
		qglOrtho                     = dllOrtho;
		qglPassThrough               = dllPassThrough;
		qglPixelMapfv                = dllPixelMapfv;
		qglPixelMapuiv               = dllPixelMapuiv;
		qglPixelMapusv               = dllPixelMapusv;
		qglPixelStoref               = dllPixelStoref;
		qglPixelStorei               = dllPixelStorei;
		qglPixelTransferf            = dllPixelTransferf;
		qglPixelTransferi            = dllPixelTransferi;
		qglPixelZoom                 = dllPixelZoom;
		qglPointSize                 = dllPointSize;
		qglPolygonMode               = dllPolygonMode;
		qglPolygonOffset             = dllPolygonOffset;
		qglPolygonStipple            = dllPolygonStipple;
		qglPopAttrib                 = dllPopAttrib;
		qglPopClientAttrib           = dllPopClientAttrib;
		qglPopMatrix                 = dllPopMatrix;
		qglPopName                   = dllPopName;
		qglPrioritizeTextures        = dllPrioritizeTextures;
		qglPushAttrib                = dllPushAttrib;
		qglPushClientAttrib          = dllPushClientAttrib;
		qglPushMatrix                = dllPushMatrix;
		qglPushName                  = dllPushName;
		qglRasterPos2d               = dllRasterPos2d;
		qglRasterPos2dv              = dllRasterPos2dv;
		qglRasterPos2f               = dllRasterPos2f;
		qglRasterPos2fv              = dllRasterPos2fv;
		qglRasterPos2i               = dllRasterPos2i;
		qglRasterPos2iv              = dllRasterPos2iv;
		qglRasterPos2s               = dllRasterPos2s;
		qglRasterPos2sv              = dllRasterPos2sv;
		qglRasterPos3d               = dllRasterPos3d;
		qglRasterPos3dv              = dllRasterPos3dv;
		qglRasterPos3f               = dllRasterPos3f;
		qglRasterPos3fv              = dllRasterPos3fv;
		qglRasterPos3i               = dllRasterPos3i;
		qglRasterPos3iv              = dllRasterPos3iv;
		qglRasterPos3s               = dllRasterPos3s;
		qglRasterPos3sv              = dllRasterPos3sv;
		qglRasterPos4d               = dllRasterPos4d;
		qglRasterPos4dv              = dllRasterPos4dv;
		qglRasterPos4f               = dllRasterPos4f;
		qglRasterPos4fv              = dllRasterPos4fv;
		qglRasterPos4i               = dllRasterPos4i;
		qglRasterPos4iv              = dllRasterPos4iv;
		qglRasterPos4s               = dllRasterPos4s;
		qglRasterPos4sv              = dllRasterPos4sv;
		qglReadBuffer                = dllReadBuffer;
		qglReadPixels                = dllReadPixels;
		qglRectd                     = dllRectd;
		qglRectdv                    = dllRectdv;
		qglRectf                     = dllRectf;
		qglRectfv                    = dllRectfv;
		qglRecti                     = dllRecti;
		qglRectiv                    = dllRectiv;
		qglRects                     = dllRects;
		qglRectsv                    = dllRectsv;
		qglRenderMode                = dllRenderMode;
		qglRotated                   = dllRotated;
		qglRotatef                   = dllRotatef;
		qglScaled                    = dllScaled;
		qglScalef                    = dllScalef;
		qglScissor                   = dllScissor;
		qglSelectBuffer              = dllSelectBuffer;
		qglShadeModel                = dllShadeModel;
		qglStencilFunc               = dllStencilFunc;
		qglStencilMask               = dllStencilMask;
		qglStencilOp                 = dllStencilOp;
		qglTexCoord1d                = dllTexCoord1d;
		qglTexCoord1dv               = dllTexCoord1dv;
		qglTexCoord1f                = dllTexCoord1f;
		qglTexCoord1fv               = dllTexCoord1fv;
		qglTexCoord1i                = dllTexCoord1i;
		qglTexCoord1iv               = dllTexCoord1iv;
		qglTexCoord1s                = dllTexCoord1s;
		qglTexCoord1sv               = dllTexCoord1sv;
		qglTexCoord2d                = dllTexCoord2d;
		qglTexCoord2dv               = dllTexCoord2dv;
		qglTexCoord2f                = dllTexCoord2f;
		qglTexCoord2fv               = dllTexCoord2fv;
		qglTexCoord2i                = dllTexCoord2i;
		qglTexCoord2iv               = dllTexCoord2iv;
		qglTexCoord2s                = dllTexCoord2s;
		qglTexCoord2sv               = dllTexCoord2sv;
		qglTexCoord3d                = dllTexCoord3d;
		qglTexCoord3dv               = dllTexCoord3dv;
		qglTexCoord3f                = dllTexCoord3f;
		qglTexCoord3fv               = dllTexCoord3fv;
		qglTexCoord3i                = dllTexCoord3i;
		qglTexCoord3iv               = dllTexCoord3iv;
		qglTexCoord3s                = dllTexCoord3s;
		qglTexCoord3sv               = dllTexCoord3sv;
		qglTexCoord4d                = dllTexCoord4d;
		qglTexCoord4dv               = dllTexCoord4dv;
		qglTexCoord4f                = dllTexCoord4f;
		qglTexCoord4fv               = dllTexCoord4fv;
		qglTexCoord4i                = dllTexCoord4i;
		qglTexCoord4iv               = dllTexCoord4iv;
		qglTexCoord4s                = dllTexCoord4s;
		qglTexCoord4sv               = dllTexCoord4sv;
		qglTexCoordPointer           = dllTexCoordPointer;
		qglTexEnvf                   = dllTexEnvf;
		qglTexEnvfv                  = dllTexEnvfv;
		qglTexEnvi                   = dllTexEnvi;
		qglTexEnviv                  = dllTexEnviv;
		qglTexGend                   = dllTexGend;
		qglTexGendv                  = dllTexGendv;
		qglTexGenf                   = dllTexGenf;
		qglTexGenfv                  = dllTexGenfv;
		qglTexGeni                   = dllTexGeni;
		qglTexGeniv                  = dllTexGeniv;
		qglTexImage1D                = dllTexImage1D;
		qglTexImage2D                = dllTexImage2D;
		qglTexParameterf             = dllTexParameterf;
		qglTexParameterfv            = dllTexParameterfv;
		qglTexParameteri             = dllTexParameteri;
		qglTexParameteriv            = dllTexParameteriv;
		qglTexSubImage1D             = dllTexSubImage1D;
		qglTexSubImage2D             = dllTexSubImage2D;
		qglTranslated                = dllTranslated;
		qglTranslatef                = dllTranslatef;
		qglVertex2d                  = dllVertex2d;
		qglVertex2dv                 = dllVertex2dv;
		qglVertex2f                  = dllVertex2f;
		qglVertex2fv                 = dllVertex2fv;
		qglVertex2i                  = dllVertex2i;
		qglVertex2iv                 = dllVertex2iv;
		qglVertex2s                  = dllVertex2s;
		qglVertex2sv                 = dllVertex2sv;
		qglVertex3d                  = dllVertex3d;
		qglVertex3dv                 = dllVertex3dv;
		qglVertex3f                  = dllVertex3f;
		qglVertex3fv                 = dllVertex3fv;
		qglVertex3i                  = dllVertex3i;
		qglVertex3iv                 = dllVertex3iv;
		qglVertex3s                  = dllVertex3s;
		qglVertex3sv                 = dllVertex3sv;
		qglVertex4d                  = dllVertex4d;
		qglVertex4dv                 = dllVertex4dv;
		qglVertex4f                  = dllVertex4f;
		qglVertex4fv                 = dllVertex4fv;
		qglVertex4i                  = dllVertex4i;
		qglVertex4iv                 = dllVertex4iv;
		qglVertex4s                  = dllVertex4s;
		qglVertex4sv                 = dllVertex4sv;
		qglVertexPointer             = dllVertexPointer;
		qglViewport                  = dllViewport;

		qglActiveTextureARB					= dllActiveTextureARB;
		qglClientActiveTextureARB			= dllClientActiveTextureARB;

		qglBindBufferARB					= dllBindBufferARB;
		qglDeleteBuffersARB					= dllDeleteBuffersARB;
		qglGenBuffersARB					= dllGenBuffersARB;
		qglBufferDataARB					= dllBufferDataARB;
		qglBufferSubDataARB					= dllBufferSubDataARB;
		qglMapBufferARB						= dllMapBufferARB;
		qglUnmapBufferARB					= dllUnmapBufferARB;

		qglVertexAttribPointerARB			= dllVertexAttribPointerARB;
		qglEnableVertexAttribArrayARB		= dllEnableVertexAttribArrayARB;
		qglDisableVertexAttribArrayARB		= dllDisableVertexAttribArrayARB;
		qglBindProgramARB					= dllBindProgramARB;
		qglDeleteProgramsARB				= dllDeleteProgramsARB;
		qglGenProgramsARB					= dllGenProgramsARB;
		qglProgramStringARB					= dllProgramStringARB;
		qglProgramEnvParameter4fARB			= dllProgramEnvParameter4fARB;
		qglProgramEnvParameter4fvARB		= dllProgramEnvParameter4fvARB;
		qglProgramLocalParameter4fARB		= dllProgramLocalParameter4fARB;
		qglProgramLocalParameter4fvARB		= dllProgramLocalParameter4fvARB;
		qglGetProgramivARB					= dllGetProgramivARB;

		qglDrawRangeElementsEXT				= dllDrawRangeElementsEXT;

		qglActiveStencilFaceEXT				= dllActiveStencilFaceEXT;

		qglDepthBoundsEXT					= dllDepthBoundsEXT;

		qwglSwapIntervalEXT					= dllSwapIntervalEXT;

		qglCombinerParameterfvNV			= dllCombinerParameterfvNV;
		qglCombinerParameterfNV				= dllCombinerParameterfNV;
		qglCombinerParameterivNV			= dllCombinerParameterivNV;
		qglCombinerParameteriNV				= dllCombinerParameteriNV;
		qglCombinerInputNV					= dllCombinerInputNV;
		qglCombinerOutputNV					= dllCombinerOutputNV;
		qglFinalCombinerInputNV				= dllFinalCombinerInputNV;

		qglGenFragmentShadersATI			= dllGenFragmentShadersATI;
		qglBindFragmentShaderATI			= dllBindFragmentShaderATI;
		qglDeleteFragmentShaderATI			= dllDeleteFragmentShaderATI;
		qglBeginFragmentShaderATI			= dllBeginFragmentShaderATI;
		qglEndFragmentShaderATI				= dllEndFragmentShaderATI;
		qglPassTexCoordATI					= dllPassTexCoordATI;
		qglSampleMapATI						= dllSampleMapATI;
		qglColorFragmentOp1ATI				= dllColorFragmentOp1ATI;
		qglColorFragmentOp2ATI				= dllColorFragmentOp2ATI;
		qglColorFragmentOp3ATI				= dllColorFragmentOp3ATI;
		qglAlphaFragmentOp1ATI				= dllAlphaFragmentOp1ATI;
		qglAlphaFragmentOp2ATI				= dllAlphaFragmentOp2ATI;
		qglAlphaFragmentOp3ATI				= dllAlphaFragmentOp3ATI;
		qglSetFragmentShaderConstantATI		= dllSetFragmentShaderConstantATI;

		qglStencilOpSeparateATI				= dllStencilOpSeparateATI;
		qglStencilFuncSeparateATI			= dllStencilFuncSeparateATI;
	}
}

/*
 =================
 QGL_LogPrintf
 =================
*/
void QGL_LogPrintf (const char *fmt, ...){

	va_list	argPtr;

	if (!glwState.logFile)
		return;

	va_start(argPtr, fmt);
	vfprintf(glwState.logFile, fmt, argPtr);
	va_end(argPtr);
}
