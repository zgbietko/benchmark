#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <string>
#include <fstream>
#include "fv_txt_utls.h"
#include "Log.h"

// WGL specific extensions for v3.0+ //////////////////////////////////////////
#ifdef _WIN32
#include <windows.h>
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
PFNWGLGETEXTENSIONSSTRINGARBPROC    pwglGetExtensionsStringARB = 0;
#define wglGetExtensionsStringARB  pwglGetExtensionsStringARB
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// function pointers for VBO Extension
// Windows needs to get function pointers from ICD OpenGL drivers,
// because opengl32.dll does not support extensions higher than v1.1.
#ifdef _WIN32
extern PFNGLGENBUFFERSARBPROC            pglGenBuffersARB = 0;             // VBO Name Generation Procedure
extern PFNGLBINDBUFFERARBPROC            pglBindBufferARB = 0;             // VBO Bind Procedure
extern PFNGLBUFFERDATAARBPROC            pglBufferDataARB = 0;             // VBO Data Loading Procedure
extern PFNGLBUFFERSUBDATAARBPROC         pglBufferSubDataARB = 0;          // VBO Sub Data Loading Procedure
extern PFNGLDELETEBUFFERSARBPROC         pglDeleteBuffersARB = 0;          // VBO Deletion Procedure
extern PFNGLGETBUFFERPARAMETERIVARBPROC  pglGetBufferParameterivARB = 0;   // return various parameters of VBO
extern PFNGLMAPBUFFERARBPROC             pglMapBufferARB = 0;              // map VBO procedure
extern PFNGLUNMAPBUFFERARBPROC           pglUnmapBufferARB = 0;            // unmap VBO procedure
#endif



const pixel_descr_t pixel_descriptors[] = {
  { 0, },
  { GL_R8,GL_RED,GL_UNSIGNED_BYTE,1,1,true,0,"PF_R8",false },
  { GL_RG8,GL_RG,GL_UNSIGNED_BYTE,2,2,true,0,"PF_RG8",false },
  { GL_RGB8,GL_RGB,GL_UNSIGNED_BYTE,3,3,true,0,"PF_RGB8",false },
  { GL_RGBA8,GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,4,4,true,PF_RGBA8_SRGB,"PF_RGBA8",false},


  { GL_SRGB8_ALPHA8,GL_RGBA,GL_UNSIGNED_BYTE,4,4,true,0,"PF_RGBA8_SRGB",false },
  { GL_SRGB8_ALPHA8,GL_BGRA,GL_UNSIGNED_BYTE,4,4,true,0,"PF_BGRA8_SRGB",false }
};

// version 2.0 or greater
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

bool initGLEW(const bool bFlag)
{
	static bool bglewIsInited = bFlag;
	if (!bglewIsInited) {
		//mfp_debug("Initializing GLEW again\n");
		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		if (GLEW_OK != err) {
			/* Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error while initializing GLEW: %s\n", glewGetErrorString(err));
			bglewIsInited = false;
		} else {
			bglewIsInited = true;
		}
	}

	return bglewIsInited;
}

bool initGLLists(const GLuint size)
{
	static GLuint g_GLList;
	if (glIsList(g_GLList) == GL_FALSE) {
		mfp_log_debug("Generating %u GL lists\n",size);
		g_GLList = glGenLists(size);
	}

	return (glIsList(g_GLList) == GL_TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// extract openGL info
// This function must be called after GL rendering context opened.
///////////////////////////////////////////////////////////////////////////////
bool glInfo::getInfo(unsigned int param)
{
    const char* str = 0;
    char* tok = 0;

    // get vendor string
    str = (const char*)glGetString(GL_VENDOR);
    if(str) this->vendor = str;                  // check NULL return value
    else return false;

    // get renderer string
    str = (const char*)glGetString(GL_RENDERER);
    if(str) this->renderer = str;                // check NULL return value
    else return false;

    // get version string
    str = (const char*)glGetString(GL_VERSION);
    if(str) this->version = str;                 // check NULL return value
    else return false;

    // get version string (v2.0+)
    str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if(str) this->glslVersion = str;            // check NULL return value
    else glslVersion = "";

    // get all extensions as a string
    str = (const char*)glGetString(GL_EXTENSIONS);

    // split extensions
    if(str)
    {
        tok = strtok((char*)str, " ");
        while(tok)
        {
            this->extensions.push_back(tok);    // put a extension into struct
            tok = strtok(0, " ");               // next token
        }
    }

    // get WGL specific extensions for v3.0+
#ifdef _WIN32 //===========================================
    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if(wglGetExtensionsStringARB && param)
    {
        str = wglGetExtensionsStringARB((HDC)param);
        if(str)
        {
            tok = strtok((char*)str, " ");
            while(tok)
            {
                this->extensions.push_back(tok);    // put a extension into struct
                tok = strtok(0, " ");               // next token
            }
        }
    }
#endif //==================================================

    // sort extension by alphabetical order
    std::sort(this->extensions.begin(), this->extensions.end());

    // get number of color bits
    glGetIntegerv(GL_RED_BITS, &this->redBits);
    glGetIntegerv(GL_GREEN_BITS, &this->greenBits);
    glGetIntegerv(GL_BLUE_BITS, &this->blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &this->alphaBits);

    // get depth bits
    glGetIntegerv(GL_DEPTH_BITS, &this->depthBits);

    // get stecil bits
    glGetIntegerv(GL_STENCIL_BITS, &this->stencilBits);

    // get max number of lights allowed
    glGetIntegerv(GL_MAX_LIGHTS, &this->maxLights);

    // get max texture resolution
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &this->maxTextureSize);

    // get max number of clipping planes
    glGetIntegerv(GL_MAX_CLIP_PLANES, &this->maxClipPlanes);

    // get max modelview and projection matrix stacks
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &this->maxModelViewStacks);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &this->maxProjectionStacks);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &this->maxAttribStacks);

    // get max texture stacks
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &this->maxTextureStacks);

    // get max uniform buffer size
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &this->maxUniformBufferSize);

    return true;
}



///////////////////////////////////////////////////////////////////////////////
// check if the video card support a certain extension
///////////////////////////////////////////////////////////////////////////////
bool glInfo::isExtensionSupported(const std::string& ext)
{
    // search corresponding extension
    std::vector<std::string>::const_iterator iter = this->extensions.begin();
    std::vector<std::string>::const_iterator endIter = this->extensions.end();

    while(iter != endIter)
    {
        if(ext == *iter)
            return true;
        else
            ++iter;
    }
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// print OpenGL info to screen and save to a file
///////////////////////////////////////////////////////////////////////////////
void glInfo::printSelf()
{
    std::stringstream ss;

    ss << std::endl; // blank line
    ss << "OpenGL Driver Info" << std::endl;
    ss << "==================" << std::endl;
    ss << "Vendor: " << this->vendor << std::endl;
    ss << "Version: " << this->version << std::endl;
    ss << "GLSL Version: " << this->glslVersion << std::endl;
    ss << "Renderer: " << this->renderer << std::endl;

    ss << std::endl;
    ss << "Color Bits(R,G,B,A): (" << this->redBits << ", " << this->greenBits
       << ", " << this->blueBits << ", " << this->alphaBits << ")\n";
    ss << "Depth Bits: " << this->depthBits << std::endl;
    ss << "Stencil Bits: " << this->stencilBits << std::endl;

    ss << std::endl;
    ss << "Max Texture Size: " << this->maxTextureSize << "x" << this->maxTextureSize << std::endl;
    ss << "Max Lights: " << this->maxLights << std::endl;
    ss << "Max Clip Planes: " << this->maxClipPlanes << std::endl;
    ss << "Max Modelview Matrix Stacks: " << this->maxModelViewStacks << std::endl;
    ss << "Max Projection Matrix Stacks: " << this->maxProjectionStacks << std::endl;
    ss << "Max Attribute Stacks: " << this->maxAttribStacks << std::endl;
    ss << "Max Texture Stacks: " << this->maxTextureStacks << std::endl;
    ss << "Max Uniform Buffer size [bytes]: " << this->maxUniformBufferSize << std::endl;
    ss << std::endl;
    ss << "Total Number of Extensions: " << this->extensions.size() << std::endl;
    ss << "==============================" << std::endl;
    for(unsigned int i = 0; i < this->extensions.size(); ++i)
        ss << this->extensions.at(i) << std::endl;

    ss << "======================================================================" << std::endl;

    std::cout << ss.str() << std::endl;
}

bool loadFile(const char* path,std::string& source)
{
	bool ret(false);
	std::ifstream srcfile(path);

	if (srcfile.is_open()) {
		source.clear();
		source = std::string(std::istreambuf_iterator<char>(srcfile),
				std::istreambuf_iterator<char>(0));
		ret = true;
	}
	return ret;
}

GLuint initShaders(const char *path_to_vertex_shader,
		           const char *path_to_fragment_shader)
{
	// Create
	GLuint vertShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	assert(vertShaderId != 0 && fragShaderId !=0);
    // Load
	std::string srcVert;
	loadFile(path_to_vertex_shader,srcVert);
	std::string srcFrag;
	loadFile(path_to_fragment_shader,srcFrag);
	// Compile
	std::cout << "Compiling vertex shader: " << path_to_vertex_shader << std::endl;

	char const * srcVertPtr = srcVert.c_str();
	glShaderSource(vertShaderId, 1, &srcVertPtr, NULL);
	glCompileShader(vertShaderId);

	GLint Result = GL_FALSE;
	int length = 0;
	// Check Vertex Shader
	glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(vertShaderId, GL_INFO_LOG_LENGTH, &length);
	if (length > 0) {
		std::vector<char> VertexShaderErrorMessage(length+1);
		glGetShaderInfoLog(vertShaderId, length, NULL, &VertexShaderErrorMessage[0]);
		std::cout << &VertexShaderErrorMessage[0] << std::endl;
	}

	std::cout << "Compiling fragment shader: " << path_to_fragment_shader << std::endl;

	char const * srcFragPtr = srcFrag.c_str();
	glShaderSource(fragShaderId, 1, &srcFragPtr, NULL);
	glCompileShader(fragShaderId);

	// Check Fragment Shader
	glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(fragShaderId, GL_INFO_LOG_LENGTH, &length);
	if (length > 0) {
		std::vector<char> FragmentShaderErrorMessage(length+1);
		glGetShaderInfoLog(fragShaderId, length, NULL, &FragmentShaderErrorMessage[0]);
		std::cout << &FragmentShaderErrorMessage[0] << std::endl;
	}

	// Link program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, vertShaderId);
	glAttachShader(ProgramID, fragShaderId);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &length);
	if (length > 0) {
		std::vector<char> ProgramErrorMessage(length+1);
		glGetProgramInfoLog(ProgramID, length, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(vertShaderId);
	glDeleteShader(fragShaderId);

	return ProgramID;
}

GLuint createBuffer(const void* data,unsigned int dataSize, GLenum target, GLenum usage)
{
	initGLEW();
    GLuint id = 0;
    glGenBuffers(1, &id);
    //mfp_debug("Generated buffer id = %u\n",id);// create a vbo
    /*if (glIsBuffer(id) == GL_FALSE) {
    	fprintf(stderr,"[createBuffer] Can't generate buffer object %u\n\n",id);
    }
    else {*/
    	glBindBuffer(target, id);                    // activate vbo id to use
    	glBufferData(target, dataSize, data, usage); // upload data to video card

    	// check data size in VBO is same as input array, if not return 0 and delete VBO
    	int bufferSize = 0;
    	glGetBufferParameteriv(target, GL_BUFFER_SIZE, &bufferSize);
    	if(dataSize != (unsigned int)bufferSize) {
    		glDeleteBuffers(1, &id);
    		id = 0;
    		fprintf(stderr,"[createBuffer] Data size is mismatch with input array\n");
    	}
  //  }
    	//mfp_debug("generated buffer: %u\n",id);
    return id;      // return VBO id
}



GLuint createTextureBuffer(const int width, const int height, const int format,
		const void* data, const unsigned buffer)

{
	const pixel_descr_t* pfd = getPixelFormatDescriptor(format);
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if(pfd->_compressed) {
		const int pitch = (width >> 2) * pfd->_size;
		const int rows = height >> 2;
		glCompressedTexImage2D(GL_TEXTURE_2D,0,pfd->_internal,width,height,0,pitch * rows,
							   buffer ? 0 : data);
	}
	else {
		glTexImage2D(GL_TEXTURE_2D,0,pfd->_internal,width,height,0,pfd->_format,pfd->_type,
					 buffer ? 0 : data);
	}

	glBindTexture(GL_TEXTURE_2D,0);
	return id;
}

GLuint createPixelBuffer(const int width, const int height, GLuint* pboId)
{
    //int ciErrNum = CL_SUCCESS;

    if (pboId) {
    	// Delete old buffer
    	//clReleaseMemObject(pbo_cl);
    	glDeleteBuffers(1, pboId);
    }

    // Create pixel buffer object for display
    *pboId = createBuffer(NULL,width * height * sizeof(GLubyte) * 4, GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

//	if( g_glInterop ) {
//		// create OpenCL buffer from GL PBO
//		pbo_cl = clCreateFromGLBuffer(cxGPUContext,CL_MEM_WRITE_ONLY, pbo, &ciErrNum);
//		oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
//	} else {
//		pbo_cl = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, width * height * sizeof(GLubyte) * 4, NULL, &ciErrNum);
//	}
//
//   // calculate new grid size
//	gridSize[0] = shrRoundUp(LOCAL_SIZE_X,width);
//	gridSize[1] = shrRoundUp(LOCAL_SIZE_Y,height);
//
//	ciErrNum |= clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void *) &pbo_cl);
//	ciErrNum |= clSetKernelArg(ckKernel, 1, sizeof(unsigned int), &width);
//	ciErrNum |= clSetKernelArg(ckKernel, 2, sizeof(unsigned int), &height);
//	oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	return 0L;
}



float getTextWidthOnScreen(const std::string& pText, void* pCurrentFont)
{
	int lViewportStats[4];
	glGetIntegerv(GL_VIEWPORT,lViewportStats);
	float lCharSizes = 0.0f;
	for(unsigned int i=0;i<pText.size();i++) {
		lCharSizes += glutBitmapWidth( pCurrentFont,pText[i]) / (float)lViewportStats[2];
	}

	return lCharSizes;
}

void drawText(const std::string& pText,float pXMin,float pYMin,float pXMax,float pYMax,int pFontSize,void* pCurrentFont,bool pAutoLineBreak,bool pFlagCentered)
{
	// We get the viewport size, to be able afterwise to transform pixel
	// sizes in "global viewport" referential.
	int lViewportStats[4];
	glGetIntegerv(GL_VIEWPORT,lViewportStats);
	const float lLineSize = ((float)pFontSize)/lViewportStats[3];

	// If enabled, we execute end-of-line cutted word processing
	std::string lWordCutContent = pText;

	std::vector<float> lVectLineWidths;

	//Word cut
	std::vector<float> lCharSizes;
	{
		for(unsigned int i=0;i<pText.size();i++) {
			lCharSizes.push_back(glutBitmapWidth(pCurrentFont,lWordCutContent[i])/(float)lViewportStats[2]);
	}}
		float lMaxWidth = pXMax-pXMin;
		float lCurrentLineWidth=0;
		float lCurrentWordWidth=0;
		int lStartOfWord=0;
		{for(unsigned int i=0;i<pText.size();i++) {
			if(pText[i] == ' ') {
				lStartOfWord = i;
				lCurrentLineWidth += lCurrentWordWidth + lCharSizes[i];
				lCurrentWordWidth=0;			
			}
			else if(pText[i] == '\n') {
				lStartOfWord = i;
				lVectLineWidths.push_back(lCurrentLineWidth+lCurrentWordWidth);
				lCurrentLineWidth=0;
				lCurrentWordWidth=0;			
			}
			else {
				lCurrentWordWidth += lCharSizes[i];	
				if(pAutoLineBreak) {
					if(lCurrentLineWidth + lCurrentWordWidth > lMaxWidth) {				
						lWordCutContent[lStartOfWord] = '\n';
						lCurrentLineWidth = 0;
					}
				}
			}
		}}
		lVectLineWidths.push_back(lCurrentLineWidth+lCurrentWordWidth);
	

	glRasterPos2f(pXMin,pYMax-lLineSize);	
	
	glPushAttrib(GL_ENABLE_BIT);
	// Text drawing

	glDisable(GL_DEPTH_TEST);

	int lCurrentLine = 1;

	bool aCenteredMode = pFlagCentered;
	if(!aCenteredMode) {
		glRasterPos2f(pXMin,pYMax-lLineSize);
	}
	else {
		glRasterPos2f((pXMax+pXMin)/2-lVectLineWidths[lCurrentLine-1]/2,pYMax-lLineSize);
	}
	float lXPos = pXMin;
	
	for(unsigned int i=0;i < lWordCutContent.size();i++) {
		lXPos += glutBitmapWidth(pCurrentFont,lWordCutContent[i])/(float)lViewportStats[2];
		if(lXPos > pXMax || lWordCutContent[i] == '\n') {
			lCurrentLine++;
			if(!aCenteredMode) {
				glRasterPos2f(pXMin,pYMax-lLineSize*lCurrentLine);
			}
			else {
				glRasterPos2f((pXMax+pXMin)/2-lVectLineWidths[lCurrentLine-1]/2,pYMax-lLineSize*lCurrentLine);
			}
			lXPos = pXMin;
			if(pYMin+lLineSize*lCurrentLine > pYMax) 
				break;			
		}
		else {
			glutBitmapCharacter(pCurrentFont,lWordCutContent[i]);
		}
	}

	glPopAttrib();

}

void drawText3D(const std::string& pText,float pX,float pY,float pZ,void* pCurrentFont)
{
	glRasterPos3f(pX,pY,pZ);	
	
	glPushAttrib(GL_ENABLE_BIT);
	// Text drawing

	glDisable(GL_DEPTH_TEST);

	
	for(unsigned int i=0;i < pText.size();i++) {
	  glutBitmapCharacter(pCurrentFont,pText[i]);
	}

	glPopAttrib();

}

void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);          // set text color
    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}

GLuint createQuadGrid(const double orig[], // minimal point of the grid
		              double dims[], // dimensions of the grid - should be 2 dims, 1 - is 0
		              double density[], // a vector of grid density
		              GLuint* nvertces // handle to number of vertices - out
		              )
{
	typedef struct {
		float x,y,z;
	} vertex_t;

	std::vector<vertex_t> lVertices;
	//std::cout << "dims before: " << dims[0] << ", " << dims[1] << ", " << dims[2] << "\n";
	// Correct density
	for (int i=0;i<3;i++)
		if (density[i] <= 0.) density[i] = 0.2;
	// Correct dims
	int index;
	for (int i=0;i<3;i++) {
		if (dims[i] == 0.) {
			index = i;
			continue;
		}
		dims[i] = ceil(dims[i]/density[i]) * density[i];
	}
	//std::cout << "dims after: " << dims[0] << ", " << dims[1] << ", " << dims[2] << "\n";
	// Generate points and theirs indices
	vertex_t vt;
	switch(index)
	{
	case 0: // yz - plane
		// y -axes
		for (double y=orig[1];y<=orig[1]+dims[1];y+=density[1]) {
			vt.x = 0.f; vt.y = y; vt.z = orig[2];
			lVertices.push_back(vt);
			vt.z += dims[2];
			lVertices.push_back(vt);
		}
		// z - axes
		for (double z=orig[2];z<=orig[2]+dims[2];z+=density[2]) {
			vt.z = z; vt.x = 0.f; vt.y = orig[1];
			lVertices.push_back(vt);
			vt.y += dims[1];
			lVertices.push_back(vt);
		}
		break;
	case 1: // xz - plane
		double x,y;
		// x -axes
		for (x=orig[0];x<=orig[0]+dims[0];x+=density[0]) {
			y = 0; vt.z = orig[2];
			lVertices.push_back(vt);
			vt.z += dims[2];
			lVertices.push_back(vt);
		}
		// z - axes
		for (double z=orig[2];z<=orig[2]+dims[2];z+=density[2]) {
			vt.z = z; vt.y = 0; vt.x = orig[1];
			lVertices.push_back(vt);
			vt.y += dims[1];
			lVertices.push_back(vt);
		}
		break;
	case 2: // xy - plane
		// x -axes
	{
		index = 0;
		double x,y;
		for (x=orig[0];x<=orig[0]+dims[0]+0.001;x+=density[0]) {
			vt.x = (float)x; vt.y = (float)orig[1]; vt.z = 0.f;
			lVertices.push_back(vt);
			//std::cout << "index = " << index++ << "(" << vt.x << ", " << vt.y << ", " << vt.z << ")\n";
			vt.y = (float)(orig[1] +dims[1]);
			lVertices.push_back(vt);
			//std::cout << "index = " << index++ << "(" << vt.x << ", " << vt.y << ", " << vt.z << ")\n";
		}
		// y - axes
		for (double y=orig[1];y<=orig[1]+dims[1]+0.001;y+=density[1]) {
			vt.x = (float)orig[0]; vt.y = (float)y; vt.z = 0.f;
			lVertices.push_back(vt);
			//std::cout << "index = " << index++ << "(" << vt.x << ", " << vt.y << ", " << vt.z << ")\n";
			vt.x = (float)(orig[0] + dims[0]);
			lVertices.push_back(vt);
			//std::cout << "index = " << index++ << "(" << vt.x << ", " << vt.y << ", " << vt.z << ")\n";
		}
	} break;
	}
	// Create buffer and fill in with data
	GLuint vboId = createBuffer(lVertices.data(),lVertices.size()*sizeof(vertex_t),GL_ARRAY_BUFFER, GL_STATIC_DRAW);
	if (vboId == 0) {
		fprintf(stderr,"Error in creating buffer for vertices of grid data\n");
		exit(-1);
	}
	*nvertces = static_cast<GLuint>(lVertices.size());
	return vboId;
}


GLuint createGrid3D(const float minb[],
		            const float maxb[],
		            const gridinfo_s* grid,
		            GLuint outBuff[])
{

	typedef struct {
		float x,y,z;
	} vertex_t;

	typedef unsigned int uint_t;
//	printf("receiv grid data: %d %d %d; %f %f %f\n",grid->cellCount.s[0],
//			grid->cellCount.s[1],grid->cellCount.s[2],grid->cellSize.s[0],
//			grid->cellSize.s[1],grid->cellSize.s[1]);
	std::vector<vertex_t> vertices;

	const vertex_t* p_min = (const vertex_t*)minb;
	const vertex_t* p_max = (const vertex_t*)maxb;
	float lengthx = p_max->x - p_min->x;
	float lengthy = p_max->y - p_min->y;
	float lengthz = p_max->z - p_min->z;
	//vtxAccum.UseVBO() = ViewManagerInst().IsVBOUsed();

	vertex_t v;
	// Add grid nodes in ZY planes
	uint_t totalNodes(0);
	for (uint_t i = 0; i <= grid->cellCount.s[0]; ++i) {
		v.x = p_min->x + i*grid->cellSize.s[0];
		for (uint_t j = 0; j <= grid->cellCount.s[1]; ++j) {
			v.y = p_min->y + j*grid->cellSize.s[1];
			v.z = p_min->z;
			vertices.push_back(v);
			v.z += lengthz;
			vertices.push_back(v);
			//std::cout << "Edge: " << totalNodes << " " << totalNodes+1 <<"\n";
			//indices.push_back(totalNodes++);
			//indices.push_back(totalNodes++);
		}
	}
	// Add grid nodes in XY planes
	for (uint_t k = 0; k <= grid->cellCount.s[2]; ++k) {
		v.z = p_min->z + k*grid->cellSize.s[2];
		for (uint_t i = 0; i <= grid->cellCount.s[0]; ++i) {
			v.x = p_min->x + i*grid->cellSize.s[0];
			v.y = p_min->y;
			vertices.push_back(v);
			v.y += lengthy;
			vertices.push_back(v);
			//std::cout << "Edge: " << totalNodes << " " << totalNodes+1 <<"\n";
			//indices.push_back(totalNodes++);
			//indices.push_back(totalNodes++);
		}
	}
	// Add grid nodes in XZ planes
	for (uint_t j = 0; j <= grid->cellCount.s[1]; ++j) {
		v.y = p_min->y + j*grid->cellSize.s[1];
		for (uint_t k = 0; k <= grid->cellCount.s[2]; ++k) {
			v.z = p_min->z + k*grid->cellSize.s[2];
			v.x= p_min->x;
			vertices.push_back(v);
			v.x += lengthx;
			vertices.push_back(v);
			//std::cout << "Edge: " << totalNodes << " " << totalNodes+1 <<"\n";
			//indices.push_back(totalNodes++);
			//indices.push_back(totalNodes++);
		}
	}

	// Create GL buffers
	GLuint vbo = createBuffer(vertices.data(), vertices.size() * sizeof(vertex_t), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
	FV_CHECK_ERROR_GL();
	//GLuint ibo = createBuffer(indices.data(), indices.size() * sizeof(uint_t), GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	//FV_CHECK_ERROR_GL();

	outBuff[0] = vbo;
	outBuff[1] = static_cast<GLuint>(vertices.size());
	//outBuff[1] = ibo;

	//printf("created: vertces: %u indices: %u ids = {%u %u}\n", vertices.size(),indices.size(),outBuff[0],outBuff[1]);

	return 0;
}

void drawGrid(const GLuint params[2],const float color[3], const float line_width)
{
	// Validate the given data
	if (glIsBuffer(params[0]) == GL_FALSE || params[1] == 0) {
		return;
	}

	if (line_width != 0.0) glLineWidth(line_width);
	// Bind data
	glColor3fv(color);
	glBindBuffer(GL_ARRAY_BUFFER, params[0]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	// Draw lines of grid
	glDrawArrays(GL_LINES, 0, params[1]);
	// Disable data
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


