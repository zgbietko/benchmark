#ifndef _FV_TXT_UTLS_H_
#define _FV_TXT_UTLS_H_

#include "types.h"
#include "fv_config.h"
#include "fv_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <vector>

#ifdef _WIN32
extern PFNGLGENBUFFERSARBPROC            pglGenBuffersARB;             // VBO Name Generation Procedure
extern PFNGLBINDBUFFERARBPROC            pglBindBufferARB;             // VBO Bind Procedure
extern PFNGLBUFFERDATAARBPROC            pglBufferDataARB;             // VBO Data Loading Procedure
extern PFNGLBUFFERSUBDATAARBPROC         pglBufferSubDataARB;          // VBO Sub Data Loading Procedure
extern PFNGLDELETEBUFFERSARBPROC         pglDeleteBuffersARB;          // VBO Deletion Procedure
extern PFNGLGETBUFFERPARAMETERIVARBPROC  pglGetBufferParameterivARB;   // return various parameters of VBO
extern PFNGLMAPBUFFERARBPROC             pglMapBufferARB;              // map VBO procedure
extern PFNGLUNMAPBUFFERARBPROC           pglUnmapBufferARB;            // unmap VBO procedure
#endif

enum ePixelFormat {
  PF_UNKNOWN = 0,
  PF_R8,
  PF_RG8,
  PF_RGB8,
  PF_RGBA8,

  // Signed and noramlized
  PF_RGBA8_SRGB,
  PF_BGRA8_SRGB,

  PF_ALL
};

/// pixel format descriptor
typedef struct {
  int _internal; 	//< OpenGL internal format (GL_RGBA8)
  int _format; 		//< OpenGL format (GL_RGBA)
  int _type; 		//< OpenGL component type (GL_UNSIGNED_BYTE)
  unsigned int _size; //< byte size of one pixel (4)
  int _components;	//< number of components (4)
  bool _rt; 		//< true if it can be used as render target
  int _sRGB; 		//< sRGB pixel format alternative
  const char *_txt; //< readable description
  bool _compressed; //< true if it is compressed format
} pixel_descr_t, *pixel_descrptr_t;

extern const pixel_descr_t pixel_descriptors[];

inline const pixel_descr_t* getPixelFormatDescriptor(const int id) {
	assert(id > PF_UNKNOWN && id < PF_ALL);
	return (pixel_descriptors + id);
}

// struct variable to store OpenGL info
struct glInfo
{
    std::string vendor;
    std::string renderer;
    std::string version;
    std::string glslVersion;
    std::vector <std::string> extensions;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int maxTextureSize;
    int maxLights;
    int maxAttribStacks;
    int maxModelViewStacks;
    int maxProjectionStacks;
    int maxClipPlanes;
    int maxTextureStacks;
    int maxUniformBufferSize;

    // ctor, init all members
    glInfo() : redBits(0), greenBits(0), blueBits(0), alphaBits(0), depthBits(0),
               stencilBits(0), maxTextureSize(0), maxLights(0), maxAttribStacks(0),
               maxModelViewStacks(0), maxClipPlanes(0), maxTextureStacks(0),
               maxUniformBufferSize(0) {}

    bool getInfo(unsigned int param=0);         // extract info
    void printSelf();                           // print itself
    bool isExtensionSupported(const std::string& ext); // check if a extension is supported
};

bool initGLEW(const bool bFlag = false);
bool initGLLists(const GLuint size);
GLuint initShaders(const char *path_to_vertex_shader, const char *payh_to_fragment_shader);
bool loadFile(const char* path,std::string& source);
GLuint createBuffer(const void* data,unsigned int dataSize, GLenum target, GLenum usage);

GLuint createTextureBuffer(const int width,
		const int height,
		const int format,
		const void* data,
		const unsigned buffer = 0
		);

GLuint createPixelBuffer(const int width,const int height,GLuint* pbId);


// Returns width of given text in current screen coordinates
float getTextWidthOnScreen(const std::string& text, void* pCurrentFont);
void  drawText(const std::string& pText,
              float pXMin, float pYMin, float pXMax, float pYMax,
              int pFontSize, void* pCurrentFont, bool pAutoLineBreak, bool pFlagCentered);

void drawText3D(const std::string& pText, float pX, float pY, float pZ, void* pCurrentFont);
void drawString(const char *str, int x, int y, float color[4], void *font);
inline bool
__CheckErrorGL(const char *file, const int line)
{
    bool ret_val = true;

    // check for error
    GLenum gl_error = glGetError();

    if (gl_error != GL_NO_ERROR)
    {
#ifdef _WIN32
        char tmpStr[512];
        // NOTE: "%s(%i) : " allows Visual Studio to directly jump to the file at the right line
        // when the user double clicks on the error line in the Output pane. Like any compile error.
        sprintf_s(tmpStr, 255, "\n%s(%i) : GL Error : %s\n\n", file, line, gluErrorString(gl_error));
        fprintf(stderr, "%s", tmpStr);
#endif
        fprintf(stderr, "GL Error in file '%s' in line %d :\n", file, line);
        fprintf(stderr, "%s\n", gluErrorString(gl_error));
        fflush(stderr);
        ret_val = false;
    }

    return ret_val;
}
#define FV_DEBUG_GL 1
#ifdef FV_DEBUG_GL
#define FV_CHECK_ERROR_GL()                                             \
    if( false == __CheckErrorGL( __FILE__, __LINE__)) {                 \
        exit(EXIT_FAILURE);												\
    }
//#ifdef FV_DEBUG_GL
//#define FV_CHECK_ERROR_GL()	\
//{ \
//	GLenum err;	\
//	if (err = glGetError()) \
//		printf("GL Error in file: %s line: %d: %s\n", __FILE__, __LINE__ ,gluErrorString(err)); \
//}
inline void print_array(const float array[],size_t size_of_array) {
	printf("\n");
	for (size_t i(0);i<size_of_array;++i)
		printf("tab[%u] = %f\n",i,array[i]);
}

inline void print_array_vec4f(const float array[],
		                      const size_t size_of_array,
		                      const size_t offset) {
	printf("\n");
	size_t i(0), j(0);
	for (; i < size_of_array; i++,j+=offset) {
		printf("vec[%u]: ",i);
		print_array(&array[j],4);
	}
}

GLuint createQuadGrid(const double orig[], // minimal point of the grid
		              double dims[], // dimensions of the grid - should be 2 dims, 1 - is 0
		              double density[], // a vector of grid density
		              GLuint* nvertces // handle to number of vertices - out
		              );
GLuint createGrid3D(const float minb[],const float maxb[],const gridinfo_s* drid, GLuint outBuff[]);
void drawGrid(const GLuint params[2],const float color[3],const float linew = 0.0);

#else
#define FV_CHECK_ERROR_GL()
#endif



#endif /* _FV_TXT_UTLS_H_
		*/
