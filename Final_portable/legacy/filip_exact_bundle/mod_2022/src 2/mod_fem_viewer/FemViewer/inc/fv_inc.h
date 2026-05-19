#ifndef _FV_INC_H_
#define _FV_INC_H_

#if defined(WIN32) && defined(_MSC_VER)
#pragma warning(disable: 4996 )
#include <windows.h>
#include <stdlib.h>
//#ifndef M_PI
//#define M_PI 3.14159265
//#endif
#endif
#include <GL/glew.h>
#include <GL/freeglut.h>

//#include <GL/gl.h>
//#include <GL/glu.h>
#ifdef _WIN32

#define glGenBuffersARB           pglGenBuffersARB
#define glBindBufferARB           pglBindBufferARB
#define glBufferDataARB           pglBufferDataARB
#define glBufferSubDataARB        pglBufferSubDataARB
#define glDeleteBuffersARB        pglDeleteBuffersARB
#define glGetBufferParameterivARB pglGetBufferParameterivARB
#define glMapBufferARB            pglMapBufferARB
#define glUnmapBufferARB          pglUnmapBufferARB
/*
#else
#define glGenBuffersARB           glGenBuffers
#define glBindBufferARB           glBindBuffer
#define glBufferDataARB           glBufferData
#define glBufferSubDataARB        glBufferSubData
#define glDeleteBuffersARB        glDeleteBuffers
#define glGetBufferParameterivARB glGetBufferParameteriv
#define glMapBufferARB            glMapBuffer
#define glUnmapBufferARB          glUnmapBuffer
*/
#endif


#ifndef FV_M_PI
#define FV_M_PI			3.14159265
#define FV_M_PIBY360	0.00872665
#endif



#endif /* _FV_INC_H_ 
*/
