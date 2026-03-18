#ifndef __PLATFORM_GL_H__
#define __PLATFORM_GL_H__

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#endif
