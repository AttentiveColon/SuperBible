#pragma once

#include "GL_Helpers.h"

struct KTX_Raw {
	KTX_Raw() :m_data(nullptr) {}
	KTX_Raw(unsigned char* data, usize data_size, int width, int height, int format, int internal_format, int gltype)
		:m_data(data), m_data_size(data_size), m_width(width), m_height(height),
		m_glformat(format), m_glinternal_format(internal_format), m_gltype(gltype)
	{}
	~KTX_Raw() { delete[] m_data; }

	unsigned char* m_data;
	usize m_data_size;
	int m_width;
	int m_height;
	int m_glformat;
	int m_glinternal_format;
	int m_gltype;
};

GLuint Load_KTX(const char* filename, GLuint texture = 0);
KTX_Raw Get_KTX_Raw(const char* filename);
GLuint CreateTextureArray(const char* filenames[], size_t length);