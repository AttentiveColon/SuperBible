#pragma once

#include "GL_Helpers.h"

GLuint Load_KTX(const char* filename, GLuint texture = 0);
unsigned char* Load_KTX_Raw(const char* filename);