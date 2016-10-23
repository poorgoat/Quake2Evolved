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


#include "../render/r_local.h"


#define WINDOW_NAME				"Quake II Evolved"
#define	WINDOW_CLASS_MAIN		"Q2E Main"
#define	WINDOW_CLASS_FAKE		"Q2E Fake"
#define WINDOW_STYLE_FULLSCREEN	(WS_POPUP)
#define WINDOW_STYLE_WINDOWED	(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU)

typedef struct {
	qboolean	accelerated;
	qboolean	drawToWindow;
	qboolean	supportOpenGL;
	qboolean	doubleBuffer;
	qboolean	rgba;
	int			colorBits;
	int			alphaBits;
	int			depthBits;
	int			stencilBits;
	int			samples;
} glwPixelFormat_t;

static int	glwNumPixelFormats = WGL_NUMBER_PIXEL_FORMATS_ARB;
static int	glwPixelFormatAttribs[11] = {
	WGL_ACCELERATION_ARB,
	WGL_DRAW_TO_WINDOW_ARB,
	WGL_SUPPORT_OPENGL_ARB,
	WGL_DOUBLE_BUFFER_ARB,
	WGL_PIXEL_TYPE_ARB,
	WGL_COLOR_BITS_ARB,
	WGL_ALPHA_BITS_ARB,
	WGL_DEPTH_BITS_ARB,
	WGL_STENCIL_BITS_ARB,
	WGL_SAMPLE_BUFFERS_ARB,
	WGL_SAMPLES_ARB
};

static const char *	(WINAPI * qwglGetExtensionsStringARB)(HDC hDC);
static BOOL			(WINAPI * qwglGetPixelFormatAttribivARB)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);

glConfig_t			glConfig;
glwState_t			glwState;


/*
 =================
 GLW_IsExtensionPresent
 =================
*/
static qboolean GLW_IsExtensionPresent (const char *extName){

	char	*extString = (char *)glConfig.extensionsString;
	char	*token;

	while (1){
		token = Com_Parse(&extString);
		if (!token[0])
			break;

		if (!Q_stricmp(token, extName))
			return true;
	}

	return false;
}

/*
 =================
 GLW_GetProcAddress
 =================
*/
static void *GLW_GetProcAddress (const char *funcName){

	void	*funcAddress;

	funcAddress = qwglGetProcAddress(funcName);
	if (!funcAddress){
		GLW_Shutdown();

		Com_Error(ERR_FATAL, "GLW_GetProcAddress: wglGetProcAddress() failed for '%s'", funcName);
	}

	return funcAddress;
}

/*
 =================
 GLW_InitExtensions
 =================
*/
static void GLW_InitExtensions (void){

	Com_Printf("Initializing OpenGL extensions\n");

	if (GLW_IsExtensionPresent("GL_ARB_multitexture")){
		glConfig.multitexture = true;

		qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxTextureUnits);

		qglActiveTextureARB = GLW_GetProcAddress("glActiveTextureARB");
		qglClientActiveTextureARB = GLW_GetProcAddress("glClientActiveTextureARB");

		Com_Printf("...using GL_ARB_multitexture\n");
	}
	else
		Com_Printf("...GL_ARB_multitexture not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_add")){
		glConfig.textureEnvAdd = true;

		Com_Printf("...using GL_ARB_texture_env_add\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_add not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_combine")){
		glConfig.textureEnvCombine = true;

		Com_Printf("...using GL_ARB_texture_env_combine\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_combine not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_env_dot3")){
		glConfig.textureEnvDot3 = true;

		Com_Printf("...using GL_ARB_texture_env_dot3\n");
	}
	else
		Com_Printf("...GL_ARB_texture_env_dot3 not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_cube_map")){
		glConfig.textureCubeMap = true;

		qglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCubeMapTextureSize);

		Com_Printf("...using GL_ARB_texture_cube_map\n");
	}
	else
		Com_Printf("...GL_ARB_texture_cube_map not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_non_power_of_two")){
		glConfig.textureNonPowerOfTwo = true;

		Com_Printf("...using GL_ARB_texture_non_power_of_two\n");
	}
	else
		Com_Printf("...GL_ARB_texture_non_power_of_two not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_compression")){
		glConfig.textureCompression = true;

		Com_Printf("...using GL_ARB_texture_compression\n");
	}
	else
		Com_Printf("...GL_ARB_texture_compression not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_compression_s3tc")) {
		glConfig.textureCompressionS3 = true;

		Com_Printf("...using GL_EXT_texture_compression_s3tc\n");
	} else
		Com_Printf("...GL_EXT_texture_compression_s3tc not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_texture_border_clamp")){
		glConfig.textureBorderClamp = true;

		Com_Printf("...using GL_ARB_texture_border_clamp\n");
	}
	else
		Com_Printf("...GL_ARB_texture_border_clamp not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_multisample")){
		glConfig.multisample = true;

		Com_Printf("...using GL_ARB_multisample\n");
	}
	else
		Com_Printf("...GL_ARB_multisample not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_vertex_buffer_object")){
		glConfig.vertexBufferObject = true;

		qglBindBufferARB = GLW_GetProcAddress("glBindBufferARB");
		qglDeleteBuffersARB = GLW_GetProcAddress("glDeleteBuffersARB");
		qglGenBuffersARB = GLW_GetProcAddress("glGenBuffersARB");
		qglBufferDataARB = GLW_GetProcAddress("glBufferDataARB");
		qglBufferSubDataARB = GLW_GetProcAddress("glBufferSubDataARB");
		qglMapBufferARB = GLW_GetProcAddress("glMapBufferARB");
		qglUnmapBufferARB = GLW_GetProcAddress("glUnmapBufferARB");

		Com_Printf("...using GL_ARB_vertex_buffer_object\n");
	}
	else
		Com_Printf("...GL_ARB_vertex_buffer_object not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_vertex_program")){
		glConfig.vertexProgram = true;

		qglVertexAttribPointerARB = GLW_GetProcAddress("glVertexAttribPointerARB");
		qglEnableVertexAttribArrayARB = GLW_GetProcAddress("glEnableVertexAttribArrayARB");
		qglDisableVertexAttribArrayARB = GLW_GetProcAddress("glDisableVertexAttribArrayARB");
		qglBindProgramARB = GLW_GetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB = GLW_GetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB = GLW_GetProcAddress("glGenProgramsARB");
		qglProgramStringARB = GLW_GetProcAddress("glProgramStringARB");
		qglProgramEnvParameter4fARB = GLW_GetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB = GLW_GetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4fARB = GLW_GetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB = GLW_GetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramivARB = GLW_GetProcAddress("glGetProgramivARB");

		Com_Printf("...using GL_ARB_vertex_program\n");
	}
	else
		Com_Printf("...GL_ARB_vertex_program not found\n");

	if (GLW_IsExtensionPresent("GL_ARB_fragment_program")){
		glConfig.fragmentProgram = true;

		qglGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &glConfig.maxTextureCoords);
		qglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.maxTextureImageUnits);

		qglBindProgramARB = GLW_GetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB = GLW_GetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB = GLW_GetProcAddress("glGenProgramsARB");
		qglProgramStringARB = GLW_GetProcAddress("glProgramStringARB");
		qglProgramEnvParameter4fARB = GLW_GetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB = GLW_GetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4fARB = GLW_GetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB = GLW_GetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramivARB = GLW_GetProcAddress("glGetProgramivARB");

		Com_Printf("...using GL_ARB_fragment_program\n");
	}
	else
		Com_Printf("...GL_ARB_fragment_program not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_draw_range_elements")){
		glConfig.drawRangeElements = true;

		qglDrawRangeElementsEXT = GLW_GetProcAddress("glDrawRangeElementsEXT");

		Com_Printf("...using GL_EXT_draw_range_elements\n");
	}
	else
		Com_Printf("...GL_EXT_draw_range_elements not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_filter_anisotropic")){
		glConfig.textureFilterAnisotropic = true;

		qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureMaxAnisotropy);

		Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
	}
	else
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_lod_bias")){
		glConfig.textureLodBias = true;

		qglGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.maxTextureLodBias);

		Com_Printf("...using GL_EXT_texture_lod_bias\n");
	}
	else
		Com_Printf("...GL_EXT_texture_lod_bias not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_texture_edge_clamp")){
		glConfig.textureEdgeClamp = true;

		Com_Printf("...using GL_EXT_texture_edge_clamp\n");
	}
	else
		Com_Printf("...GL_EXT_texture_edge_clamp not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_bgra")){
		glConfig.bgra = true;

		Com_Printf("...using GL_EXT_bgra\n");
	}
	else
		Com_Printf("...GL_EXT_bgra not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_stencil_wrap")){
		glConfig.stencilWrap = true;

		Com_Printf("...using GL_EXT_stencil_wrap\n");
	}
	else
		Com_Printf("...GL_EXT_stencil_wrap not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_stencil_two_side")){
		glConfig.stencilTwoSide = true;

		qglActiveStencilFaceEXT = GLW_GetProcAddress("glActiveStencilFaceEXT");

		Com_Printf("...using GL_EXT_stencil_two_side\n");
	}
	else
		Com_Printf("...GL_EXT_stencil_two_side not found\n");

	if (GLW_IsExtensionPresent("GL_EXT_depth_bounds_test")){
		glConfig.depthBoundsTest = true;

		qglDepthBoundsEXT = GLW_GetProcAddress("glDepthBoundsEXT");

		Com_Printf("...using GL_EXT_depth_bounds_test\n");
	}
	else
		Com_Printf("...GL_EXT_depth_bounds_test not found\n");

	if (GLW_IsExtensionPresent("WGL_EXT_swap_control")){
		glConfig.swapControl = true;

		qwglSwapIntervalEXT = GLW_GetProcAddress("wglSwapIntervalEXT");

		Com_Printf("...using WGL_EXT_swap_control\n");
	}
	else
		Com_Printf("...WGL_EXT_swap_control not found\n");

	if (GLW_IsExtensionPresent("GL_NV_register_combiners")){
		glConfig.nvRegisterCombiners = true;

		qglCombinerParameterfvNV = GLW_GetProcAddress("glCombinerParameterfvNV");
		qglCombinerParameterfNV = GLW_GetProcAddress("glCombinerParameterfNV");
		qglCombinerParameterivNV = GLW_GetProcAddress("glCombinerParameterivNV");
		qglCombinerParameteriNV = GLW_GetProcAddress("glCombinerParameteriNV");
		qglCombinerInputNV = GLW_GetProcAddress("glCombinerInputNV");
		qglCombinerOutputNV = GLW_GetProcAddress("glCombinerOutputNV");
		qglFinalCombinerInputNV = GLW_GetProcAddress("glFinalCombinerInputNV");

		Com_Printf("...using GL_NV_register_combiners\n");
	}
	else
		Com_Printf("...GL_NV_register_combiners not found\n");

	if (GLW_IsExtensionPresent("GL_ATI_fragment_shader")){
		glConfig.atiFragmentShader = true;

		qglGenFragmentShadersATI = GLW_GetProcAddress("glGenFragmentShadersATI");
		qglBindFragmentShaderATI = GLW_GetProcAddress("glBindFragmentShaderATI");
		qglDeleteFragmentShaderATI = GLW_GetProcAddress("glDeleteFragmentShaderATI");
		qglBeginFragmentShaderATI = GLW_GetProcAddress("glBeginFragmentShaderATI");
		qglEndFragmentShaderATI = GLW_GetProcAddress("glEndFragmentShaderATI");
		qglPassTexCoordATI = GLW_GetProcAddress("glPassTexCoordATI");
		qglColorFragmentOp1ATI = GLW_GetProcAddress("glColorFragmentOp1ATI");
		qglColorFragmentOp2ATI = GLW_GetProcAddress("glColorFragmentOp2ATI");
		qglColorFragmentOp3ATI = GLW_GetProcAddress("glColorFragmentOp3ATI");
		qglAlphaFragmentOp1ATI = GLW_GetProcAddress("glAlphaFragmentOp1ATI");
		qglAlphaFragmentOp2ATI = GLW_GetProcAddress("glAlphaFragmentOp2ATI");
		qglAlphaFragmentOp3ATI = GLW_GetProcAddress("glAlphaFragmentOp3ATI");
		qglSetFragmentShaderConstantATI = GLW_GetProcAddress("glSetFragmentShaderConstantATI");

		Com_Printf("...using GL_ATI_fragment_shader\n");
	}
	else
		Com_Printf("...GL_ATI_fragment_shader not found\n");

	if (GLW_IsExtensionPresent("GL_ATI_separate_stencil")){
		glConfig.atiSeparateStencil = true;

		qglStencilOpSeparateATI = GLW_GetProcAddress("glStencilOpSeparateATI");
		qglStencilFuncSeparateATI = GLW_GetProcAddress("glStencilFuncSeparateATI");

		Com_Printf("...using GL_ATI_separate_stencil\n");
	}
	else
		Com_Printf("...GL_ATI_separate_stencil not found\n");
}

/*
 =================
 GLW_InitFakeOpenGL

 Fake OpenGL stuff to work around the crappy WGL limitations.
 Do this silently.
 =================
*/
static void GLW_InitFakeOpenGL (void){

	WNDCLASSEX				wndClass;
	RECT					r;
	PIXELFORMATDESCRIPTOR	PFD;
	int						pixelFormat;

	r.left = 0;
	r.top = 0;
	r.right = 320;
	r.bottom = 240;

	AdjustWindowRect(&r, WINDOW_STYLE_WINDOWED, FALSE);

	// Register the frame class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = (WNDPROC)FakeWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = LoadIcon(sys.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm = 0;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
	wndClass.lpszMenuName = 0;
	wndClass.lpszClassName = WINDOW_CLASS_FAKE;

	if (!RegisterClassEx(&wndClass))
		return;

	// Create the fake window
	if ((glwState.hWndFake = CreateWindowEx(0, WINDOW_CLASS_FAKE, WINDOW_NAME, WINDOW_STYLE_WINDOWED, 3, 22, r.right - r.left, r.bottom - r.top, NULL, NULL, sys.hInstance, NULL)) == NULL){
		UnregisterClass(WINDOW_CLASS_FAKE, sys.hInstance);
		return;
	}

	// Get a DC for the fake window
	if ((glwState.hDCFake = GetDC(glwState.hWndFake)) == NULL)
		return;

	// Choose a pixel format
	memset(&PFD, 0, sizeof(PIXELFORMATDESCRIPTOR));

	PFD.cColorBits = 32;
	PFD.cDepthBits = 24;
	PFD.cStencilBits = 8;
	PFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	PFD.iLayerType = PFD_MAIN_PLANE;
	PFD.iPixelType = PFD_TYPE_RGBA;
	PFD.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	PFD.nVersion = 1;

	pixelFormat = ChoosePixelFormat(glwState.hDCFake, &PFD);
	if (!pixelFormat)
		return;

	// Set the pixel format
	DescribePixelFormat(glwState.hDCFake, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &PFD);

	if (!SetPixelFormat(glwState.hDCFake, pixelFormat, &PFD))
		return;

	// Create the fake GL context and make it current
	if ((glwState.hGLRCFake = qwglCreateContext(glwState.hDCFake)) == NULL)
		return;

    if (!qwglMakeCurrent(glwState.hDCFake, glwState.hGLRCFake))
		return;

	// We only need this function pointer, if available
	qwglGetPixelFormatAttribivARB = qwglGetProcAddress("wglGetPixelFormatAttribivARB");
}

/*
 =================
 GLW_ShutdownFakeOpenGL

 Fake OpenGL stuff to work around the crappy WGL limitations.
 Do this silently.
 =================
*/
static void GLW_ShutdownFakeOpenGL (void){

	if (glwState.hGLRCFake){
		if (qwglMakeCurrent)
			qwglMakeCurrent(NULL, NULL);

		if (qwglDeleteContext)
			qwglDeleteContext(glwState.hGLRCFake);

		glwState.hGLRCFake = NULL;
	}

	if (glwState.hDCFake){
		ReleaseDC(glwState.hWndFake, glwState.hDCFake);
		glwState.hDCFake = NULL;
	}

	if (glwState.hWndFake){
		DestroyWindow(glwState.hWndFake);
		glwState.hWndFake = NULL;

		UnregisterClass(WINDOW_CLASS_FAKE, sys.hInstance);
	}

	qwglGetPixelFormatAttribivARB = NULL;
}

/*
 =================
 GLW_ChoosePixelFormat
 =================
*/
static int GLW_ChoosePixelFormat (int colorBits, int alphaBits, int depthBits, int stencilBits, int samples){

	PIXELFORMATDESCRIPTOR	PFD;
	int						values[12];
	glwPixelFormat_t		current, selected;
	int						i, numPixelFormats, pixelFormat = 0;

	// Initialize the fake OpenGL stuff so that we can use the extended
	// pixel format functionality
	GLW_InitFakeOpenGL();

	// Get number of pixel formats
	if (qwglGetPixelFormatAttribivARB)
		qwglGetPixelFormatAttribivARB(glwState.hDC, 0, 0, 1, &glwNumPixelFormats, &numPixelFormats);
	else
		numPixelFormats = DescribePixelFormat(glwState.hDC, 0, 0, NULL);

	if (numPixelFormats < 1){
		Com_Printf("...no pixel formats found\n");

		GLW_ShutdownFakeOpenGL();

		return 0;
	}

	Com_Printf("...%i pixel formats found\n", numPixelFormats);

	// Report if multisampling is desired
	if (samples)
		Com_Printf("...attempting to use multisampling\n");

	// Run through all the pixel formats, looking for the best match
	for (i = 1; i <= numPixelFormats; i++){
		// Describe the current pixel format
		if (qwglGetPixelFormatAttribivARB){
			if (!qwglGetPixelFormatAttribivARB(glwState.hDC, i, 0, 11, glwPixelFormatAttribs, values)){
				// If failed, WGL_ARB_multisample is not supported
				qwglGetPixelFormatAttribivARB(glwState.hDC, i, 0, 9, glwPixelFormatAttribs, values);

				values[9] = GL_FALSE;
				values[10] = 0;
			}

			current.accelerated = (values[0] == WGL_FULL_ACCELERATION_ARB);
			current.drawToWindow = (values[1] == GL_TRUE);
			current.supportOpenGL = (values[2] == GL_TRUE);
			current.doubleBuffer = (values[3] == GL_TRUE);
			current.rgba = (values[4] == WGL_TYPE_RGBA_ARB);
			current.colorBits = values[5];
			current.alphaBits = values[6];
			current.depthBits = values[7];
			current.stencilBits = values[8];
			current.samples = (values[9] == GL_TRUE) ? values[10] : 0;
		}
		else {
			DescribePixelFormat(glwState.hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &PFD);

			current.accelerated = !(PFD.dwFlags & PFD_GENERIC_FORMAT);
			current.drawToWindow = (PFD.dwFlags & PFD_DRAW_TO_WINDOW);
			current.supportOpenGL = (PFD.dwFlags & PFD_SUPPORT_OPENGL);
			current.doubleBuffer = (PFD.dwFlags & PFD_DOUBLEBUFFER);
			current.rgba = (PFD.iPixelType == PFD_TYPE_RGBA);
			current.colorBits = PFD.cColorBits;
			current.alphaBits = PFD.cAlphaBits;
			current.depthBits = PFD.cDepthBits;
			current.stencilBits = PFD.cStencilBits;
			current.samples = 0;
		}

		// Check acceleration
		if (!current.accelerated){
			Com_DPrintf("...PIXELFORMAT %i rejected, software emulation\n", i);
			continue;
		}

		// Check multisamples
		if (current.samples && !samples){
			Com_DPrintf("...PIXELFORMAT %i rejected, multisample\n", i);
			continue;
		}

		// Check flags
		if (!current.drawToWindow || !current.supportOpenGL || !current.doubleBuffer){
			Com_DPrintf("...PIXELFORMAT %i rejected, improper flags\n", i);
			continue;
		}

		// Check pixel type
		if (!current.rgba){
			Com_DPrintf("...PIXELFORMAT %i rejected, not RGBA\n", i);
			continue;
		}

		// Check color bits
		if (current.colorBits < colorBits){
			Com_DPrintf("...PIXELFORMAT %i rejected, insufficient color bits (%i < %i)\n", i, current.colorBits, colorBits);
			continue;
		}

		// Check alpha bits
		if (current.alphaBits < alphaBits){
			Com_DPrintf("...PIXELFORMAT %i rejected, insufficient alpha bits (%i < %i)\n", i, current.alphaBits, alphaBits);
			continue;
		}

		// Check depth bits
		if (current.depthBits < depthBits){
			Com_DPrintf("...PIXELFORMAT %i rejected, insufficient depth bits (%i < %i)\n", i, current.depthBits, depthBits);
			continue;
		}

		// Check stencil bits
		if (current.stencilBits < stencilBits){
			Com_DPrintf("...PIXELFORMAT %i rejected, insufficient stencil bits (%i < %i)\n", i, current.stencilBits, stencilBits);
			continue;
		}

		// If we don't have a selected pixel format yet, then use it
		if (!pixelFormat){
			selected = current;
			pixelFormat = i;
			continue;
		}

		// If current pixel format is better than selected pixel format,
		// then use it
		if (selected.samples != samples){
			if (current.samples == samples || current.samples > selected.samples){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (selected.colorBits != colorBits){
			if (current.colorBits == colorBits || current.colorBits > selected.colorBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (selected.alphaBits != alphaBits){
			if (current.alphaBits == alphaBits || current.alphaBits > selected.alphaBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (selected.depthBits != depthBits){
			if (current.depthBits == depthBits || current.depthBits > selected.depthBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}

		if (selected.stencilBits != stencilBits){
			if (current.stencilBits == stencilBits || current.stencilBits > selected.stencilBits){
				selected = current;
				pixelFormat = i;
				continue;
			}
		}
	}

	// Shutdown the fake OpenGL stuff since we no longer need it
	GLW_ShutdownFakeOpenGL();

	// Make sure we have a valid pixel format
	if (!pixelFormat){
		Com_Printf("...no hardware acceleration found\n");
		return 0;
	}

	Com_Printf("...hardware acceleration found\n");

	// Report if multisampling is desired but unavailable
	if (samples && !selected.samples)
		Com_Printf("...failed to select a multisample PIXELFORMAT\n");

	glConfig.colorBits = selected.colorBits;
	glConfig.alphaBits = selected.alphaBits;
	glConfig.depthBits = selected.depthBits;
	glConfig.stencilBits = selected.stencilBits;
	glConfig.samples = selected.samples;

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
	
	Com_Printf("Initializing OpenGL driver\n");

	// Get a DC for the current window
	Com_Printf("...getting DC: ");
	if ((glwState.hDC = GetDC(glwState.hWnd)) == NULL){
		Com_Printf("failed\n" );
		return false;
	}
	Com_Printf("succeeded\n");

	// Get the device gamma ramp
	if (glwState.allowDeviceGammaRamp){
		Com_Printf("...getting gamma ramp: ");
		if ((glConfig.deviceSupportsGamma = GetDeviceGammaRamp(glwState.hDC, glwState.deviceGammaRamp)) == false)
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");
	}
	else
		glConfig.deviceSupportsGamma = false;

	// Choose a pixel format
	pixelFormat = GLW_ChoosePixelFormat(32, 8, 24, 8, r_multiSamples->integerValue);
	if (!pixelFormat){
		Com_Printf("...failed to find an appropriate PIXELFORMAT\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	// Set the pixel format
	DescribePixelFormat(glwState.hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &PFD);

	if (!SetPixelFormat(glwState.hDC, pixelFormat, &PFD)){
		Com_Printf("...SetPixelFormat failed\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	// Create the GL context and make it current
	Com_Printf("...creating GL context: ");
	if ((glwState.hGLRC = qwglCreateContext(glwState.hDC)) == NULL){
		Com_Printf("failed\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}
	Com_Printf("succeeded\n");

	Com_Printf("...making context current: ");
    if (!qwglMakeCurrent(glwState.hDC, glwState.hGLRC)){
		Com_Printf("failed\n");

		qwglDeleteContext(glwState.hGLRC);
		glwState.hGLRC = NULL;

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}
	Com_Printf("succeeded\n");

	return true;
}

/*
 =================
 GLW_CreateWindow
 =================
*/
static qboolean GLW_CreateWindow (void){

	WNDCLASSEX	wndClass;
	RECT		r;
	int			style, exStyle;
	int			x, y, w, h;

	if (r_fullscreen->integerValue){
		exStyle = WS_EX_TOPMOST;
		style = WINDOW_STYLE_FULLSCREEN;

		x = 0;
		y = 0;
	}
	else {
		exStyle = 0;
		style = WINDOW_STYLE_WINDOWED;

		x = Cvar_GetInteger("vid_xPos");
		y = Cvar_GetInteger("vid_yPos");
	}

	r.left = 0;
	r.top = 0;
	r.right = glConfig.videoWidth;
	r.bottom = glConfig.videoHeight;

	AdjustWindowRect(&r, style, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	// Register the frame class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = (WNDPROC)MainWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = LoadIcon(sys.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm = 0;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
	wndClass.lpszMenuName = 0;
	wndClass.lpszClassName = WINDOW_CLASS_MAIN;

	if (!RegisterClassEx(&wndClass)){
		Com_Printf("...failed to register window class\n");
		return false;
	}
	Com_Printf("...registered window class\n");

	// Create the window
	if ((glwState.hWnd = CreateWindowEx(exStyle, WINDOW_CLASS_MAIN, WINDOW_NAME, style, x, y, w, h, NULL, NULL, sys.hInstance, NULL)) == NULL){
		Com_Printf("...failed to create window\n");

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);
		return false;
	}
	Com_Printf("...created window @ %i,%i (%ix%i)\n", x, y, w, h);

	ShowWindow(glwState.hWnd, SW_SHOW);
	UpdateWindow(glwState.hWnd);
	SetForegroundWindow(glwState.hWnd);
	SetFocus(glwState.hWnd);

	if (!GLW_InitDriver()){
		Com_Printf("...destroying window\n");

		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);
		return false;
	}

	return true;
}

/*
 =================
 GLW_SetDisplayMode
 =================
*/
static qboolean GLW_SetDisplayMode (void){

	DEVMODE	devMode;
	HDC		hDC;
	int		displayDepth, displayRefresh;
	int		cdsErr;

	Com_Printf("...setting mode %i: ", r_mode->integerValue);
	if (!R_GetModeInfo(r_mode->integerValue, &glConfig.videoWidth, &glConfig.videoHeight)){
		Com_Printf("invalid mode\n");
		return false;
	}

	Com_Printf("%i %i ", glConfig.videoWidth, glConfig.videoHeight);

	if (r_fullscreen->integerValue){
		if (r_displayRefresh->integerValue)
			Com_Printf("FS (%i Hz)\n", r_displayRefresh->integerValue);
		else
			Com_Printf("FS\n");
	}
	else
		Com_Printf("W\n");

	// Do a CDS if needed
	if (!r_fullscreen->integerValue){
		Com_Printf("...setting windowed mode\n");
		ChangeDisplaySettings(0, 0);

		hDC = GetDC(0);
		displayDepth = GetDeviceCaps(hDC, BITSPIXEL);
		displayRefresh = GetDeviceCaps(hDC, VREFRESH);
		ReleaseDC(0, hDC);
		if (displayDepth != 32){
			Com_Printf("...windowed mode requires 32 bits desktop depth\n");
			return false;
		}

		glConfig.isFullscreen = false;
		glConfig.displayFrequency = displayRefresh;

		if (!GLW_CreateWindow())
			return false;

		return true;
	}

	memset(&devMode, 0, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	devMode.dmPelsWidth = glConfig.videoWidth;
	devMode.dmPelsHeight = glConfig.videoHeight;
	devMode.dmDisplayFrequency = r_displayRefresh->integerValue;
	devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	if (glwState.allowDisplayDepthChange){
		devMode.dmBitsPerPel = 32;
		devMode.dmFields |= DM_BITSPERPEL;
	}
	else {
		hDC = GetDC(0);
		displayDepth = GetDeviceCaps(hDC, BITSPIXEL);
		ReleaseDC(0, hDC);

		if (displayDepth != 32){
			Com_Printf("...running on Win95 < OSR2 requires 32 bits desktop depth\n");
			return false;
		}
	}

	Com_Printf("...calling CDS: ");

	if ((cdsErr = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL){
		Com_Printf("ok\n");

		hDC = GetDC(0);
		displayRefresh = GetDeviceCaps(hDC, VREFRESH);
		ReleaseDC(0, hDC);

		glConfig.isFullscreen = true;
		glConfig.displayFrequency = displayRefresh;

		if (!GLW_CreateWindow()){
			Com_Printf("...restoring display settings\n");

			ChangeDisplaySettings(0, 0);
			glConfig.isFullscreen = false;

			return false;
		}

		return true;
	}

	switch (cdsErr){
	case DISP_CHANGE_BADFLAGS:
		Com_Printf("bad flags\n");
		break;
	case DISP_CHANGE_BADMODE:
		Com_Printf("bad mode\n");
		break;
	case DISP_CHANGE_BADPARAM:
		Com_Printf("bad param\n");
		break;
	case DISP_CHANGE_NOTUPDATED:
		Com_Printf("not updated\n");
		break;
	case DISP_CHANGE_RESTART:
		Com_Printf("restart required\n");
		break;
	case DISP_CHANGE_FAILED:
		Com_Printf("failed\n");
		break;
	default:
		Com_Printf("unknown error (%i)\n", cdsErr);
		break;
	}

	return false;
}

/*
 =================
 GLW_StartOpenGL
 =================
*/
static void GLW_StartOpenGL (void){

	// Initialize our QGL dynamic bindings
	if (!QGL_Init(r_glDriver->value))
		Com_Error(ERR_FATAL, "GLW_StartOpenGL: could not load OpenGL subsystem");

	// Initialize the display, window, context, etc...
	if (!GLW_SetDisplayMode()){
		QGL_Shutdown();

		Com_Error(ERR_FATAL, "GLW_StartOpenGL: could not load OpenGL subsystem");
	}

	// Get GL strings
	glConfig.vendorString = qglGetString(GL_VENDOR);
	glConfig.rendererString = qglGetString(GL_RENDERER);
	glConfig.versionString = qglGetString(GL_VERSION);
	glConfig.extensionsString = qglGetString(GL_EXTENSIONS);

	// Get WGL strings, if possible
	qwglGetExtensionsStringARB = qwglGetProcAddress("wglGetExtensionsStringARB");
	if (qwglGetExtensionsStringARB)
		glConfig.wglExtensionsString = qwglGetExtensionsStringARB(glwState.hDC);
	else
		glConfig.wglExtensionsString = "";

	// Get max texture size supported
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize);
}

/*
 =================
 GLW_CheckOSVersion
 =================
*/
static void GLW_CheckOSVersion (void){

#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	osInfo;

	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osInfo))
		Com_Error(ERR_FATAL, "GLW_CheckOSVersion: GetVersionEx() failed");

	if (osInfo.dwMajorVersion > 4){
		glwState.allowDisplayDepthChange = true;
		glwState.allowDeviceGammaRamp = true;
	}
	else if (osInfo.dwMajorVersion == 4){
		if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			glwState.allowDisplayDepthChange = true;
		else if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS){
			if (LOWORD(osInfo.dwBuildNumber) >= OSR2_BUILD_NUMBER)
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
void GLW_SetDeviceGammaRamp (const byte *gammaTable){

	unsigned short	gammaRamp[3][256];
	unsigned short	v;
	int				i;

	if (!glConfig.deviceSupportsGamma)
		return;

	if (!glwState.hDC)
		return;

	for (i = 0; i < 256; i++){
		v = (((unsigned short)gammaTable[i]) << 8) | gammaTable[i];

		gammaRamp[0][i] = v;
		gammaRamp[1][i] = v;
		gammaRamp[2][i] = v;
	}

	if (!SetDeviceGammaRamp(glwState.hDC, gammaRamp))
		Com_Printf(S_COLOR_YELLOW "WARNING: SetDeviceGammaRamp() failed\n");
}

/*
 =================
 GLW_ActivateContext
 =================
*/
void GLW_ActivateContext (qboolean active){

	if (!glwState.hDC || !glwState.hGLRC)
		return;

	if (active){
		if (!qwglMakeCurrent(glwState.hDC, glwState.hGLRC))
			Com_Error(ERR_FATAL, "GLW_ActivateContext: wglMakeCurrent() failed");
	}
	else {
		if (!qwglMakeCurrent(NULL, NULL))
			Com_Error(ERR_FATAL, "GLW_ActivateContext: wglMakeCurrent() failed");
	}
}

/*
 =================
 GLW_SwapBuffers
 =================
*/
void GLW_SwapBuffers (void){

	if (r_swapInterval->modified){
		if (r_swapInterval->integerValue < 0)
			Cvar_SetInteger("r_swapInterval", 0);

		if (glConfig.swapControl)
			qwglSwapIntervalEXT(r_swapInterval->integerValue);

		r_swapInterval->modified = false;
	}

	if (!r_frontBuffer->integerValue){
		if (!qwglSwapBuffers(glwState.hDC))
			Com_Error(ERR_FATAL, "GLW_SwapBuffers: wglSwapBuffers() failed");
	}
}

/*
 =================
 GLW_Init
 =================
*/
void GLW_Init (void){

	qboolean	unsupported = false;

	Com_Printf("Initializing OpenGL subsystem\n");

	// Check OS version
	GLW_CheckOSVersion();

	// Initialize OpenGL subsystem
	GLW_StartOpenGL();

	// Initialize extensions
	GLW_InitExtensions();

	// Make sure the required extensions are available
	if (!glConfig.multitexture)
		unsupported = true;
	if (!glConfig.textureEnvAdd)
		unsupported = true;
	if (!glConfig.textureEnvCombine)
		unsupported = true;
	if (!glConfig.textureEnvDot3)
		unsupported = true;
	if (!glConfig.textureCubeMap)
		unsupported = true;
	if (!glConfig.textureEdgeClamp)
		unsupported = true;
	if (!glConfig.bgra)
		unsupported = true;
	if (!glConfig.stencilWrap)
		unsupported = true;

	if (unsupported){
		GLW_Shutdown();

		Com_Error(ERR_FATAL, "The current video card / driver combination does not support the necessary features");
	}

	// Look for available rendering paths
	if (glConfig.nvRegisterCombiners){
		glConfig.allowNV10Path = true;

		if (glConfig.vertexProgram && glConfig.maxTextureUnits >= 4)
			glConfig.allowNV20Path = true;

		if (glConfig.vertexProgram && glConfig.maxTextureUnits >= 4)
			glConfig.allowNV30Path = true;
	}

	if (glConfig.atiFragmentShader){
		if (glConfig.vertexProgram && glConfig.maxTextureUnits >= 6)
			glConfig.allowR200Path = true;
	}

	if (glConfig.vertexProgram && glConfig.fragmentProgram)
		glConfig.allowARB2Path = true;

	// Enable logging if requested
	QGL_EnableLogging(r_logFile->integerValue);
}

/*
 =================
 GLW_Shutdown
 =================
*/
void GLW_Shutdown (void){

	Com_Printf("Shutting down OpenGL subsystem\n");

	if (glwState.hGLRC){
		if (qwglMakeCurrent){
			Com_Printf("...wglMakeCurrent( NULL, NULL ): ");
			if (!qwglMakeCurrent(NULL, NULL))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}
		
		if (qwglDeleteContext){
			Com_Printf("...deleting GL context: ");
			if (!qwglDeleteContext(glwState.hGLRC))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}

		glwState.hGLRC = NULL;
	}

	if (glwState.hDC){
		if (glConfig.deviceSupportsGamma){
			Com_Printf("...restoring gamma ramp: ");
			if (!SetDeviceGammaRamp(glwState.hDC, glwState.deviceGammaRamp))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}

		Com_Printf("...releasing DC: ");
		if (!ReleaseDC(glwState.hWnd, glwState.hDC))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		glwState.hDC = NULL;
	}

	if (glwState.hWnd){
		Com_Printf("...destroying window\n");

		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);
	}

	if (glConfig.isFullscreen){
		Com_Printf("...restoring display settings\n");

		ChangeDisplaySettings(0, 0);
		glConfig.isFullscreen = false;
	}

	QGL_EnableLogging(false);

	QGL_Shutdown();

	memset(&glConfig, 0, sizeof(glConfig_t));
	memset(&glwState, 0, sizeof(glwState_t));
}
