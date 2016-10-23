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


#include "../refresh/r_local.h"


#define	WINDOW_CLASS_NAME	"Q2E"
#define WINDOW_NAME			"Quake 2 Evolved"

typedef enum {
	ERR_OK,
	ERR_INVALID_FULLSCREEN,
	ERR_INVALID_MODE
} glwModeErr_t;

glConfig_t	glConfig;
glwState_t	glwState;


/*
 =================
 GLW_InitDeviceGammaRamp
 =================
*/
static void GLW_InitDeviceGammaRamp (void){

	// Check if the user wants to ignore it
	if (r_ignoreHwGamma->integer){
		glConfig.deviceSupportsGamma = false;
		return;
	}

	// Check if not allowed on this platform
	if (!glwState.allowDeviceGammaRamp){
		glConfig.deviceSupportsGamma = false;
		return;
	}

	// Get original gamma ramp
	glConfig.deviceSupportsGamma = GetDeviceGammaRamp(glwState.hDC, glwState.deviceGammaRamp);
	if (!glConfig.deviceSupportsGamma){
		Com_DPrintf(S_COLOR_YELLOW "WARNING: GetDeviceGammaRamp() failed\n");
		return;
	}
}

/*
 =================
 GLW_IsExtensionPresent
 =================
*/
static qboolean GLW_IsExtensionPresent (const char *extName){

	char	*extString;
	char	*tok;

	extString = (char *)glConfig.extensionsString;
	while (1){
		tok = Com_ParseExt(&extString, false);
		if (!tok[0])
			break;

		if (!Q_stricmp(tok, extName))
			return true;
	}

	return false;
}

/*
 =================
 GLW_InitExtensions
 =================
*/
static void GLW_InitExtensions (void){

	if (!r_allowExtensions->integer){
		Com_Printf("*** IGNORING OPENGL EXTENSIONS ***\n");
		return;
	}

	Com_Printf("Initializing OpenGL extensions\n");

	if (GLW_IsExtensionPresent("GL_ARB_multitexture")){
		if (r_arb_multitexture->integer){
			glConfig.multitexture = true;

			qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxTextureUnits);

			qglActiveTextureARB = qwglGetProcAddress("glActiveTextureARB");
			qglClientActiveTextureARB = qwglGetProcAddress("glClientActiveTextureARB");

			Com_Printf("...using GL_ARB_multitexture\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_multitexture\n");
	}
	else
		Com_Printf("...GL_ARB_multitexture not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_add")){
		if (r_arb_texture_env_add->integer){
			glConfig.textureEnvAdd = true;

			Com_Printf("...using GL_ARB_texture_env_add\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_texture_env_add\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_add not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_combine")){
		if (r_arb_texture_env_combine->integer){
			glConfig.textureEnvCombine = true;

			Com_Printf("...using GL_ARB_texture_env_combine\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_texture_env_combine\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_combine not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_dot3")){
		if (r_arb_texture_env_dot3->integer){
			glConfig.textureEnvDot3 = true;

			Com_Printf("...using GL_ARB_texture_env_dot3\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_texture_env_dot3\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_dot3 not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_cube_map")){
		if (r_arb_texture_cube_map->integer){
			glConfig.textureCubeMap = true;

			qglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCubeMapTextureSize);

			Com_Printf("...using GL_ARB_texture_cube_map\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_texture_cube_map\n");
	}
	else
		Com_Printf("...GL_ARB_texture_cube_map not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_compression")){
		if (r_arb_texture_compression->integer){
			glConfig.textureCompression = true;

			Com_Printf("...using GL_ARB_texture_compression\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_texture_compression\n");
	}
	else
		Com_Printf("...GL_ARB_texture_compression not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_vertex_buffer_object")){
		if (r_arb_vertex_buffer_object->integer){
			glConfig.vertexBufferObject = true;

			qglBindBufferARB = qwglGetProcAddress("glBindBufferARB");
			qglDeleteBuffersARB = qwglGetProcAddress("glDeleteBuffersARB");
			qglGenBuffersARB = qwglGetProcAddress("glGenBuffersARB");
			qglBufferDataARB = qwglGetProcAddress("glBufferDataARB");
			qglBufferSubDataARB = qwglGetProcAddress("glBufferSubDataARB");
			qglMapBufferARB = qwglGetProcAddress("glMapBufferARB");
			qglUnmapBufferARB = qwglGetProcAddress("glUnmapBufferARB");

			Com_Printf("...using GL_ARB_vertex_buffer_object\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_vertex_buffer_object\n");
	}
	else
		Com_Printf("...GL_ARB_vertex_buffer_object not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_vertex_program")){
		if (r_arb_vertex_program->integer){
			glConfig.vertexProgram = true;

			qglBindProgramARB = qwglGetProcAddress("glBindProgramARB");
			qglDeleteProgramsARB = qwglGetProcAddress("glDeleteProgramsARB");
			qglGenProgramsARB = qwglGetProcAddress("glGenProgramsARB");
			qglProgramStringARB = qwglGetProcAddress("glProgramStringARB");
			qglProgramEnvParameter4fARB = qwglGetProcAddress("glProgramEnvParameter4fARB");
			qglProgramLocalParameter4fARB = qwglGetProcAddress("glProgramLocalParameter4fARB");

			Com_Printf("...using GL_ARB_vertex_program\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_vertex_program\n");
	}
	else
		Com_Printf("...GL_ARB_vertex_program not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_fragment_program")){
		if (r_arb_fragment_program->integer){
			glConfig.fragmentProgram = true;

			qglGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &glConfig.maxTextureCoords);
			qglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.maxTextureImageUnits);

			qglBindProgramARB = qwglGetProcAddress("glBindProgramARB");
			qglDeleteProgramsARB = qwglGetProcAddress("glDeleteProgramsARB");
			qglGenProgramsARB = qwglGetProcAddress("glGenProgramsARB");
			qglProgramStringARB = qwglGetProcAddress("glProgramStringARB");
			qglProgramEnvParameter4fARB = qwglGetProcAddress("glProgramEnvParameter4fARB");
			qglProgramLocalParameter4fARB = qwglGetProcAddress("glProgramLocalParameter4fARB");

			Com_Printf("...using GL_ARB_fragment_program\n");
		}
		else
			Com_Printf("...ignoring GL_ARB_fragment_program\n");
	}
	else
		Com_Printf("...GL_ARB_fragment_program not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_draw_range_elements")){
		if (r_ext_draw_range_elements->integer){
			glConfig.drawRangeElements = true;

			qglDrawRangeElementsEXT = qwglGetProcAddress("glDrawRangeElementsEXT");

			Com_Printf("...using GL_EXT_draw_range_elements\n");
		}
		else
			Com_Printf("...ignoring GL_EXT_draw_range_elements\n");
	}
	else
		Com_Printf("...GL_EXT_draw_range_elements not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_compiled_vertex_array")){
		if (r_ext_compiled_vertex_array->integer){
			glConfig.compiledVertexArray = true;

			qglLockArraysEXT = qwglGetProcAddress("glLockArraysEXT");
			qglUnlockArraysEXT = qwglGetProcAddress("glUnlockArraysEXT");

			Com_Printf("...using GL_EXT_compiled_vertex_array\n");
		}
		else
			Com_Printf("...ignoring GL_EXT_compiled_vertex_array\n");
	}
	else
		Com_Printf("...GL_EXT_compiled_vertex_array not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_edge_clamp")){
		if (r_ext_texture_edge_clamp->integer){
			glConfig.textureEdgeClamp = true;

			Com_Printf("...using GL_EXT_texture_edge_clamp\n");
		}
		else
			Com_Printf("...ignoring GL_EXT_texture_edge_clamp\n");
	}
	else
		Com_Printf("...GL_EXT_texture_edge_clamp not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_filter_anisotropic")){
		if (r_ext_texture_filter_anisotropic->integer){
			glConfig.textureFilterAnisotropic = true;

			qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureMaxAnisotropy);

			Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
		}
		else
			Com_Printf("...ignoring GL_EXT_texture_filter_anisotropic\n");
	}
	else
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");

	if (GLW_IsExtensionPresent("GL_NV_texture_rectangle") || GLW_IsExtensionPresent("GL_EXT_texture_rectangle")){
		if (r_ext_texture_rectangle->integer){
			glConfig.textureRectangle = true;

			qglGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &glConfig.maxRectangleTextureSize);

			Com_Printf("...using GL_NV_texture_rectangle\n");
		}
		else
			Com_Printf("...ignoring GL_NV_texture_rectangle\n");
	}
	else
		Com_Printf("...GL_NV_texture_rectangle not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_stencil_two_side")){
		if (r_ext_stencil_two_side->integer){
			glConfig.stencilTwoSide = true;

			qglActiveStencilFaceEXT = qwglGetProcAddress("glActiveStencilFaceEXT");

			Com_Printf("...using GL_EXT_stencil_two_side\n");
		}
		else
			Com_Printf("...ignoring GL_EXT_stencil_two_side\n");
	}
	else if (GLW_IsExtensionPresent("GL_ATI_separate_stencil")){
		if (r_ext_stencil_two_side->integer){
			glConfig.separateStencil = true;

			qglStencilOpSeparateATI = qwglGetProcAddress("glStencilOpSeparateATI");
			qglStencilFuncSeparateATI = qwglGetProcAddress("glStencilFuncSeparateATI");

			Com_Printf("...using GL_ATI_separate_stencil\n");
		}
		else
			Com_Printf("...ignoring GL_ATI_separate_stencil\n");
	}
	else
		Com_Printf("...GL_EXT_stencil_two_side not found\n");

	if (GLW_IsExtensionPresent("GL_SGIS_generate_mipmap")){
		if (r_ext_generate_mipmap->integer){
			glConfig.generateMipmap = true;

			Com_Printf("...using GL_SGIS_generate_mipmap\n");
		}
		else
			Com_Printf("...ignoring GL_SGIS_generate_mipmap\n");
	}
	else
		Com_Printf("...GL_SGIS_generate_mipmap not found\n");

	if (GLW_IsExtensionPresent("WGL_EXT_swap_control")){
		if (r_ext_swap_control->integer){
			glConfig.swapControl = true;

			qwglSwapIntervalEXT = qwglGetProcAddress("wglSwapIntervalEXT");

			Com_Printf("...using WGL_EXT_swap_control\n");
		}
		else
			Com_Printf("...ignoring WGL_EXT_swap_control\n");
	}
	else
		Com_Printf("...WGL_EXT_swap_control not found\n");
}

/*
 =================
 GLW_ChoosePFD
 =================
*/
static int GLW_ChoosePFD (int colorBits, int depthBits, int stencilBits){

#define MAX_PFDS		256

	PIXELFORMATDESCRIPTOR	PFDs[MAX_PFDS], *current, *selected;
	int						numPFDs, pixelFormat = 0;
	unsigned				flags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	int						i;

	Com_Printf("...GLW_ChoosePFD( %i, %i, %i )\n", colorBits, depthBits, stencilBits);

	// Count PFDs
	if (glConfig.miniDriver)
		numPFDs = qwglDescribePixelFormat(glwState.hDC, 0, 0, NULL);
	else
		numPFDs = DescribePixelFormat(glwState.hDC, 0, 0, NULL);

	if (numPFDs > MAX_PFDS){
		Com_Printf("...numPFDs > MAX_PFDS (%i > %i)\n", numPFDs, MAX_PFDS);
		numPFDs = MAX_PFDS;
	}
	else if (numPFDs < 1){
		Com_Printf("...GLW_ChoosePFD failed\n");
		return 0;
	}

	Com_Printf("...%i PFDs found\n", numPFDs);

	// Run through all the PFDs, looking for the best match
	for (i = 1, current = PFDs; i <= numPFDs; i++, current++){
		if (glConfig.miniDriver)
			qwglDescribePixelFormat(glwState.hDC, i, sizeof(PIXELFORMATDESCRIPTOR), current);
		else
			DescribePixelFormat(glwState.hDC, i, sizeof(PIXELFORMATDESCRIPTOR), current);

		// Check acceleration
		if ((current->dwFlags & PFD_GENERIC_FORMAT) && !r_allowSoftwareGL->integer){
			Com_DPrintf("...PFD %i rejected, software acceleration\n", i);
			continue;
		}

		// Check stereo
		if ((current->dwFlags & PFD_STEREO) && !r_stereo->integer){
			Com_DPrintf("...PFD %i rejected, stereo\n", i);
			continue;
		}

		// Check flags
		if ((current->dwFlags & flags) != flags){
			Com_DPrintf("...PFD %i rejected, improper flags (0x%x instead of 0x%x)\n", i, current->dwFlags, flags);
			continue;
		}

		// Check pixel type
		if (current->iPixelType != PFD_TYPE_RGBA){
			Com_DPrintf("...PFD %i rejected, not RGBA\n", i);
			continue;
		}

		// Check color bits
		if (current->cColorBits < colorBits){
			Com_DPrintf("...PFD %i rejected, insufficient color bits (%i < %i)\n", i, current->cColorBits, colorBits);
			continue;
		}

		// Check depth bits
		if (current->cDepthBits < depthBits){
			Com_DPrintf("...PFD %i rejected, insufficient depth bits (%i < %i)\n", i, current->cDepthBits, depthBits);
			continue;
		}

		// Check stencil bits
		if (current->cStencilBits < stencilBits){
			Com_DPrintf("...PFD %i rejected, insufficient stencil bits (%i < %i)\n", i, current->cStencilBits, stencilBits);
			continue;
		}

		// If we don't have a selected PFD yet, then use it
		if (!pixelFormat){
			selected = current;
			pixelFormat = i;
			continue;
		}

		// If current PFD is better than selected PFD, then use it
		if (r_stereo->integer && !(selected->dwFlags & PFD_STEREO)){
			if (current->dwFlags & PFD_STEREO){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (colorBits != selected->cColorBits){
			if (colorBits == current->cColorBits || current->cColorBits > selected->cColorBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (depthBits != selected->cDepthBits){
			if (depthBits == current->cDepthBits || current->cDepthBits > selected->cDepthBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (stencilBits != selected->cStencilBits){
			if (stencilBits == current->cStencilBits || current->cStencilBits > selected->cStencilBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}
	}

	if (!pixelFormat){
		Com_Printf("...no hardware acceleration found\n");
		return 0;
	}

	if (selected->dwFlags & PFD_GENERIC_FORMAT){
		if (selected->dwFlags & PFD_GENERIC_ACCELERATED)
			Com_Printf("...MCD acceleration found\n");
		else
			Com_Printf("...using software emulation\n");
	}
	else
		Com_Printf("...hardware acceleration found\n");

	Com_Printf("...PIXELFORMAT %i selected\n", pixelFormat);

	return pixelFormat;
}

/*
 =================
 GLW_InitDriver
 =================
*/
static qboolean GLW_InitDriver (void){

	PIXELFORMATDESCRIPTOR	PFD;
	int						pixelFormat;
	int						colorBits, depthBits, stencilBits;
	
	Com_Printf("Initializing OpenGL driver\n");

	// Get a DC for the current window
	Com_Printf("...getting DC: ");
	if ((glwState.hDC = GetDC(glwState.hWnd)) == NULL){
		Com_Printf("failed\n" );
		return false;
	}
	Com_Printf("succeeded\n");

	// Report if stereo is desired
	if (r_stereo->integer)
		Com_Printf("...attempting to use stereo\n");

	// Set color/depth/stencil
	colorBits = (r_colorBits->integer) ? r_colorBits->integer : 32;
	depthBits = (r_depthBits->integer) ? r_depthBits->integer : 24;
	stencilBits = (r_stencilBits->integer) ? r_stencilBits->integer : 0;

	// Choose a pixel format
	pixelFormat = GLW_ChoosePFD(colorBits, depthBits, stencilBits);
	if (!pixelFormat){
		// Try again with default color/depth/stencil
		if (colorBits > 16 || depthBits > 16 || stencilBits > 0)
			pixelFormat = GLW_ChoosePFD(16, 16, 0);
		else
			pixelFormat = GLW_ChoosePFD(32, 24, 0);

		if (!pixelFormat){
			Com_Printf("...failed to find an appropriate PIXELFORMAT\n");
			
			ReleaseDC(glwState.hWnd, glwState.hDC);
			glwState.hDC = NULL;

			return false;
		}
	}

	// Set the pixel format
	if (glConfig.miniDriver){
		qwglDescribePixelFormat(glwState.hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &PFD);

		if (!qwglSetPixelFormat(glwState.hDC, pixelFormat, &PFD)){
			Com_Printf("...wglSetPixelFormat failed\n");
			goto failed;
		}
	}
	else {
		DescribePixelFormat(glwState.hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &PFD);

		if (!SetPixelFormat(glwState.hDC, pixelFormat, &PFD)){
			Com_Printf("...SetPixelFormat failed\n");
			goto failed;
		}
	}

	// Report if stereo is desired but unavailable
	if (r_stereo->integer && !(PFD.dwFlags & PFD_STEREO))
		Com_Printf("...failed to select stereo PIXELFORMAT\n");

	glConfig.colorBits = PFD.cColorBits;
	glConfig.depthBits = PFD.cDepthBits;
	glConfig.stencilBits = PFD.cStencilBits;
	glConfig.stereoEnabled = (PFD.dwFlags & PFD_STEREO);

	// Create the GL context and make it current
	Com_Printf("...creating GL context: ");
	if ((glwState.hGLRC = qwglCreateContext(glwState.hDC)) == NULL){
		Com_Printf("failed\n");
		goto failed;
	}
	Com_Printf("succeeded\n");

	Com_Printf("...making context current: ");
    if (!qwglMakeCurrent(glwState.hDC, glwState.hGLRC)){
		Com_Printf("failed\n");
		goto failed;
	}
	Com_Printf("succeeded\n");

	return true;

failed:

	Com_Printf("...failed hard\n");

	if (glwState.hGLRC){
		qwglDeleteContext(glwState.hGLRC);
		glwState.hGLRC = NULL;
	}

	if (glwState.hDC){
		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;
	}

	return false;
}

/*
 =================
 GLW_CreateWindow
 =================
*/
static qboolean GLW_CreateWindow (qboolean fullscreen){

	WNDCLASSEX	wndClass;
	RECT		r;
	int			style, exStyle;
	int			x, y, w, h;

	if (fullscreen){
		exStyle = WS_EX_TOPMOST;
		style = WS_POPUP | WS_VISIBLE;

		x = 0;
		y = 0;
	}
	else {
		exStyle = 0;
		style = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

		x = Cvar_VariableInteger("vid_xPos");
		y = Cvar_VariableInteger("vid_yPos");
	}

	r.left = 0;
	r.top = 0;
	r.right = glConfig.videoWidth;
	r.bottom = glConfig.videoHeight;

	AdjustWindowRect(&r, style, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	// Register the frame class
	wndClass.style			= 0;
	wndClass.lpfnWndProc	= (WNDPROC)glwState.WndProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= glwState.hInstance;
	wndClass.hIcon			= LoadIcon(glwState.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm		= 0;
	wndClass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground	= (HBRUSH)COLOR_GRAYTEXT;
	wndClass.lpszMenuName	= 0;
	wndClass.lpszClassName	= WINDOW_CLASS_NAME;
	wndClass.cbSize			= sizeof(WNDCLASSEX);

	if (!RegisterClassEx(&wndClass)){
		Com_Printf("...could not register window class\n");
		return false;
	}
	Com_Printf("...registered window class\n");

	// Create the window
	if ((glwState.hWnd = CreateWindowEx(exStyle, WINDOW_CLASS_NAME, WINDOW_NAME, style, x, y, w, h, NULL, NULL, glwState.hInstance, NULL)) == NULL){
		Com_Printf("...could not create window\n");

		UnregisterClass(WINDOW_CLASS_NAME, glwState.hInstance);
		return false;
	}
	Com_Printf("...created window@%i,%i (%ix%i)\n", x, y, w, h);

	ShowWindow(glwState.hWnd, SW_SHOW);
	UpdateWindow(glwState.hWnd);
	SetForegroundWindow(glwState.hWnd);
	SetFocus(glwState.hWnd);

	if (!GLW_InitDriver()){
		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_NAME, glwState.hInstance);
		return false;
	}

	return true;
}

/*
 =================
 GLW_SetMode
 =================
*/
static glwModeErr_t GLW_SetMode (qboolean fullscreen, int colorBits){

	DEVMODE	devMode;
	HDC		hDC;
	int		cdsErr;

	Com_Printf("...setting mode %i: ", r_mode->integer);
	if (!R_GetModeInfo(&glConfig.videoWidth, &glConfig.videoHeight, r_mode->integer)){
		Com_Printf("invalid mode\n");
		return ERR_INVALID_MODE;
	}

	Com_Printf("%i %i ", glConfig.videoWidth, glConfig.videoHeight);

	if (fullscreen){
		if (r_displayRefresh->integer)
			Com_Printf("FS (%i Hz)\n", r_displayRefresh->integer);
		else
			Com_Printf("FS\n");
	}
	else
		Com_Printf("W\n");

	if (!fullscreen){
		Com_Printf("...setting windowed mode\n");
		ChangeDisplaySettings(0, 0);

		hDC = GetDC(0);
		glConfig.displayDepth = GetDeviceCaps(hDC, BITSPIXEL);
		glConfig.displayFrequency = GetDeviceCaps(hDC, VREFRESH);
		glConfig.isFullscreen = false;
		ReleaseDC(0, hDC);

		if (!GLW_CreateWindow(false))
			return ERR_INVALID_MODE;

		return ERR_OK;
	}

	// Do a CDS
	memset(&devMode, 0, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	devMode.dmPelsWidth = glConfig.videoWidth;
	devMode.dmPelsHeight = glConfig.videoHeight;
	devMode.dmDisplayFrequency = r_displayRefresh->integer;
	devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	if (colorBits && glwState.allowDisplayDepthChange){
		devMode.dmBitsPerPel = colorBits;
		devMode.dmFields |= DM_BITSPERPEL;
		Com_Printf("...using color bits of %i\n", colorBits);
	}
	else {
		if (colorBits)
			Com_Printf("...WARNING: changing depth not supported on Win95 < OSR2\n");

		hDC = GetDC(0);
		Com_Printf("...using desktop display depth of %i\n", GetDeviceCaps(hDC, BITSPIXEL));
		ReleaseDC(0, hDC);
	}

	Com_Printf("...calling CDS: ");

	if ((cdsErr = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL){
		Com_Printf("ok\n");

		hDC = GetDC(0);
		glConfig.displayDepth = GetDeviceCaps(hDC, BITSPIXEL);
		glConfig.displayFrequency = GetDeviceCaps(hDC, VREFRESH);
		glConfig.isFullscreen = true;
		ReleaseDC(0, hDC);

		if (!GLW_CreateWindow(true)){
			Com_Printf("...restoring display settings\n");
			ChangeDisplaySettings(0, 0);

			return ERR_INVALID_MODE;
		}

		return ERR_OK;
	}

	switch (cdsErr){
	case DISP_CHANGE_BADFLAGS:		Com_Printf("bad flags\n");					break;
	case DISP_CHANGE_BADMODE:		Com_Printf("bad mode\n");					break;
	case DISP_CHANGE_BADPARAM:		Com_Printf("bad param\n");					break;
	case DISP_CHANGE_NOTUPDATED:	Com_Printf("not updated\n");				break;
	case DISP_CHANGE_RESTART:		Com_Printf("restart required\n");			break;
	case DISP_CHANGE_FAILED:		Com_Printf("failed\n");						break;
	default:						Com_Printf("unknown error (%i)\n", cdsErr);	break;
	}

	Com_Printf("...trying next higher resolution: ");
	if (!R_GetModeInfo(&glConfig.videoWidth, &glConfig.videoHeight, r_mode->integer + 1)){
		Com_Printf("invalid mode\n");
		return ERR_INVALID_MODE;
	}

	devMode.dmPelsWidth = glConfig.videoWidth;
	devMode.dmPelsHeight = glConfig.videoHeight;
	devMode.dmDisplayFrequency = r_displayRefresh->integer;
	devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	if (colorBits && glwState.allowDisplayDepthChange){
		devMode.dmBitsPerPel = colorBits;
		devMode.dmFields |= DM_BITSPERPEL;
	}

	if ((cdsErr = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL){
		Com_Printf("ok\n");

		hDC = GetDC(0);
		glConfig.displayDepth = GetDeviceCaps(hDC, BITSPIXEL);
		glConfig.displayFrequency = GetDeviceCaps(hDC, VREFRESH);
		glConfig.isFullscreen = true;
		ReleaseDC(0, hDC);

		if (!GLW_CreateWindow(true)){
			Com_Printf("...restoring display settings\n");
			ChangeDisplaySettings(0, 0);

			return ERR_INVALID_MODE;
		}

		return ERR_OK;
	}
	
	switch (cdsErr){
	case DISP_CHANGE_BADFLAGS:		Com_Printf("bad flags\n");					break;
	case DISP_CHANGE_BADMODE:		Com_Printf("bad mode\n");					break;
	case DISP_CHANGE_BADPARAM:		Com_Printf("bad param\n");					break;
	case DISP_CHANGE_NOTUPDATED:	Com_Printf("not updated\n");				break;
	case DISP_CHANGE_RESTART:		Com_Printf("restart required\n");			break;
	case DISP_CHANGE_FAILED:		Com_Printf("failed\n");						break;
	default:						Com_Printf("unknown error (%i)\n", cdsErr);	break;
	}

	return ERR_INVALID_FULLSCREEN;
}

/*
 =================
 GLW_StartOpenGL
 =================
*/
static qboolean GLW_StartOpenGL (const char *driver){

	glwModeErr_t	err;;

	// Initialize our QGL dynamic bindings
	if (!Q_stricmp(driver, GL_DRIVER_OPENGL))
		glConfig.miniDriver = false;
	else {
		if (r_maskMiniDriver->integer)
			glConfig.miniDriver = false;
		else {
			glConfig.miniDriver = true;

			Com_Printf("...assuming '%s' is a standalone driver\n", driver);
		}
	}

	if (!QGL_Init(driver))
		return false;

	// Initialize the display, window, context, etc...
	if ((err = GLW_SetMode(r_fullscreen->integer, r_colorBits->integer)) == ERR_OK)
		return true;
	
	if (err == ERR_INVALID_FULLSCREEN){
		Com_Printf("...WARNING: fullscreen unavailable in this mode (%i)\n", r_mode->integer);

		if ((err = GLW_SetMode(false, r_colorBits->integer)) == ERR_OK)
			return true;
	}
	else if (err == ERR_INVALID_MODE){
		Com_Printf("...WARNING: could not set the given mode (%i)\n", r_mode->integer);

		if (r_colorBits->integer == 32){
			if ((err = GLW_SetMode(r_fullscreen->integer, 16)) == ERR_OK)
				return true;
		}
		else if (r_colorBits->integer == 16){
			if ((err = GLW_SetMode(r_fullscreen->integer, 32)) == ERR_OK)
				return true;
		}
		else {
			if ((err = GLW_SetMode(r_fullscreen->integer, 0)) == ERR_OK)
				return true;
		}
	}

	if (err == ERR_INVALID_FULLSCREEN)
		Com_Printf("...WARNING: fullscreen unavailable in this mode (%i)\n", r_mode->integer);
	else if (err == ERR_INVALID_MODE)
		Com_Printf("...WARNING: could not set the given mode (%i)\n", r_mode->integer);

	// Shutdown QGL
	QGL_Shutdown();
	
	return false;
}

/*
 =================
 GLW_CheckOSVersion
 =================
*/
static void GLW_CheckOSVersion (void){

#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vInfo;

	vInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&vInfo))
		Com_Error(ERR_FATAL, "GLW_CheckOSVersion: GetVersionEx() failed");

	if (vInfo.dwMajorVersion > 4){
		glwState.allowDisplayDepthChange = true;
		glwState.allowDeviceGammaRamp = true;
	}
	else if (vInfo.dwMajorVersion == 4){
		if (vInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			glwState.allowDisplayDepthChange = true;
		else if (vInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS){
			if (LOWORD(vInfo.dwBuildNumber) >= OSR2_BUILD_NUMBER)
				glwState.allowDisplayDepthChange = true;

			glwState.allowDeviceGammaRamp = true;
		}
	}
}

/*
 =================
 GLW_SetDeviceGammaRamp
 =================
*/
void GLW_SetDeviceGammaRamp (unsigned short *gammaRamp){

	if (!glConfig.deviceSupportsGamma)
		return;

	if (!glwState.hDC)
		return;

	if (!SetDeviceGammaRamp(glwState.hDC, gammaRamp))
		Com_DPrintf(S_COLOR_YELLOW "WARNING: SetDeviceGammaRamp() failed\n");
}

/*
 =================
 GLW_SwapBuffers
 =================
*/
void GLW_SwapBuffers (void){

	if (r_swapInterval->modified){
		if (r_swapInterval->integer < 0)
			Cvar_SetInteger("r_swapInterval", 0);

		if (!glConfig.stereoEnabled){
			if (glConfig.swapControl)
				qwglSwapIntervalEXT(r_swapInterval->integer);
		}

		r_swapInterval->modified = false;
	}

	if (!r_frontBuffer->integer){
		if (!qwglSwapBuffers(glwState.hDC))
			Com_Error(ERR_FATAL, "GLW_SwapBuffers: SwapBuffers() failed");
	}
}

/*
 =================
 GLW_Activate
 =================
*/
void GLW_Activate (qboolean active){

	if (active){
		ShowWindow(glwState.hWnd, SW_RESTORE);
		SetForegroundWindow(glwState.hWnd);

		GLW_SetDeviceGammaRamp(glState.gammaRamp);
	}
	else {
		GLW_SetDeviceGammaRamp(glwState.deviceGammaRamp);

		if (glConfig.isFullscreen)
			ShowWindow(glwState.hWnd, SW_MINIMIZE);
	}
}

/*
 =================
 GLW_Init
 =================
*/
void GLW_Init (void){

	Com_Printf("Initializing OpenGL subsystem\n");

	glwState.hInstance = sys_hInstance;
	glwState.WndProc = MainWndProc;

	// Check OS version
	GLW_CheckOSVersion();

	// Initialize OpenGL subsystem
	if (!GLW_StartOpenGL(r_glDriver->string))
		Com_Error(ERR_FATAL, "GLW_StartOpenGL: could not load OpenGL subsystem");

	// Get GL strings
	glConfig.vendorString = qglGetString(GL_VENDOR);
	glConfig.rendererString = qglGetString(GL_RENDERER);
	glConfig.versionString = qglGetString(GL_VERSION);
	glConfig.extensionsString = qglGetString(GL_EXTENSIONS);

	// Get max texture size supported
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize);

	// Initialize extensions
	GLW_InitExtensions();

	// Initialize device gamma ramp
	GLW_InitDeviceGammaRamp();

	// Enable logging if requested
	QGL_EnableLogging(r_logFile->integer);
}

/*
 =================
 GLW_Shutdown
 =================
*/
void GLW_Shutdown (void){

	Com_Printf("Shutting down OpenGL subsystem\n");

	if (glConfig.deviceSupportsGamma){
		if (glwState.hDC)
			SetDeviceGammaRamp(glwState.hDC, glwState.deviceGammaRamp);
	}

	if (glwState.hGLRC){
		if (qwglMakeCurrent){
			Com_Printf("...wglMakeCurrent( NULL, NULL ): ");
			if (!qwglMakeCurrent(NULL, NULL))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}
		
		if (qwglDeleteContext){
			Com_Printf("...deletting GL context: ");
			if (!qwglDeleteContext(glwState.hGLRC))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}

		glwState.hGLRC = NULL;
	}

	if (glwState.hDC){
		Com_Printf("...releasing DC: ");
		if (!ReleaseDC(glwState.hWnd, glwState.hDC))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		glwState.hDC   = NULL;
	}

	if (glwState.hWnd){
		Com_Printf("...destroying window\n");

		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_NAME, glwState.hInstance);
	}

	if (glConfig.isFullscreen){
		Com_Printf("...resetting display\n");

		ChangeDisplaySettings(0, 0);
		glConfig.isFullscreen = false;
	}

	QGL_EnableLogging(false);

	QGL_Shutdown();

	memset(&glConfig, 0, sizeof(glConfig_t));
	memset(&glwState, 0, sizeof(glwState_t));
}
