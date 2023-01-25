#pragma once

#include "Defines.h"

typedef signed char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef long long isize;
typedef unsigned long long usize;

typedef float f32;
typedef double f64;

#define I8_MIN         (-127i8 - 1)
#define I16_MIN        (-32767i16 - 1)
#define I32_MIN        (-2147483647i32 - 1)
#define I64_MIN        (-9223372036854775807i64 - 1)
#define I8_MAX         127i8
#define I16_MAX        32767i16
#define I32_MAX        2147483647i32
#define I64_MAX        9223372036854775807i64
#define U8_MAX        0xffui8
#define U16_MAX       0xffffui16
#define U32_MAX       0xffffffffui32
#define U64_MAX       0xffffffffffffffffui64

struct GLFWimage;
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef char GLchar;

struct ShaderFiles {
    GLenum type;
    const char* filename;
    GLuint shader;
};

struct ShaderText {
    GLenum type;
    const char* shader_text;
    GLuint shader;
};

GLuint LoadShaders(ShaderText*);
GLuint LoadShaders(ShaderFiles*);
size_t GetGLTypeSize(GLenum type);
GLFWimage rgba_bmp_load(const char* filePath);
void Debug_Callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);