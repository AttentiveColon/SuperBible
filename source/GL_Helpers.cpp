#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GL_Helpers.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>

std::string ReadShader(const char* filename) {
	std::ifstream ifs;
	ifs.open(filename);

	if (!ifs.is_open()) {
		std::cerr << "Unable to open file '" << filename << "'" << std::endl;
		return std::string("");
	}

	std::stringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

std::string ReadShaderText(const char* shader_text) {
	std::stringstream ss;
	ss << shader_text;
	return ss.str();
}

GLuint LoadShaders(ShaderText* shaders) {
	if (shaders == nullptr) return 0;

	GLuint program = glCreateProgram();

	ShaderText* entry = shaders;
	while (entry->type != GL_NONE) {
		GLuint shader = glCreateShader(entry->type);
		entry->shader = shader;

		std::string source = ReadShaderText(entry->shader_text);
		if (source.empty()) {
			for (entry = shaders; entry->type != GL_NONE; ++entry) {
				glDeleteShader(entry->shader);
				entry->shader = 0;
			}
			return 0;
		}

		GLchar const* shader_source = source.c_str();
		GLint const shader_length = static_cast<GLint>(source.size());

		glShaderSource(shader, 1, &shader_source, &shader_length);
		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
#ifdef _DEBUG
			GLsizei len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

			GLchar* log = new GLchar[len + 1];
			glGetShaderInfoLog(shader, len, &len, log);
			std::cerr << "Shader compilation failed: " << log << std::endl;
			delete[] log;
#endif //_DEBUG
			return 0;
		}

		glAttachShader(program, shader);
		++entry;
	}

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
#ifdef _DEBUG
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = new GLchar[len + 1];
		glGetProgramInfoLog(program, len, &len, log);
		std::cerr << "Shader linking failed: " << log << std::endl;
		delete[] log;
#endif //_DEBUG

		for (entry = shaders; entry->type != GL_NONE; ++entry) {
			glDeleteShader(entry->shader);
			entry->shader = 0;
		}
		return 0;
	}
	return program;
}

GLuint LoadShaders(ShaderFiles* shaders) {
	if (shaders == nullptr) return 0;

	GLuint program = glCreateProgram();

	ShaderFiles* entry = shaders;
	while (entry->type != GL_NONE) {
		GLuint shader = glCreateShader(entry->type);
		entry->shader = shader;

		std::string source = ReadShader(entry->filename);
		if (source.empty()) {
			for (entry = shaders; entry->type != GL_NONE; ++entry) {
				glDeleteShader(entry->shader);
				entry->shader = 0;
			}
			return 0;
		}

		GLchar const* shader_source = source.c_str();
		GLint const shader_length = static_cast<GLint>(source.size());

		glShaderSource(shader, 1, &shader_source, &shader_length);
		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
#ifdef _DEBUG
			GLsizei len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

			GLchar* log = new GLchar[len + 1];
			glGetShaderInfoLog(shader, len, &len, log);
			std::cerr << "Shader compilation failed: " << log << std::endl;
			delete[] log;
#endif //_DEBUG
			return 0;
		}

		glAttachShader(program, shader);
		++entry;
	}

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
#ifdef _DEBUG
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = new GLchar[len + 1];
		glGetProgramInfoLog(program, len, &len, log);
		std::cerr << "Shader linking failed: " << log << std::endl;
		delete[] log;
#endif //_DEBUG

		for (entry = shaders; entry->type != GL_NONE; ++entry) {
			glDeleteShader(entry->shader);
			entry->shader = 0;
		}
		return 0;
	}
	return program;
}

size_t GetGLTypeSize(GLenum type)
{
	size_t size;

#define CASE(Enum, Count, Type) \
case Enum: size = Count * sizeof(Type); break

	switch (type) {
		CASE(GL_FLOAT, 1, GLfloat);
		CASE(GL_FLOAT_VEC2, 2, GLfloat);
		CASE(GL_FLOAT_VEC3, 3, GLfloat);
		CASE(GL_FLOAT_VEC4, 4, GLfloat);
		CASE(GL_INT, 1, GLint);
		CASE(GL_INT_VEC2, 2, GLint);
		CASE(GL_INT_VEC3, 3, GLint);
		CASE(GL_INT_VEC4, 4, GLint);
		CASE(GL_UNSIGNED_INT, 1, GLuint);
		CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
		CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
		CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
		CASE(GL_BOOL, 1, GLboolean);
		CASE(GL_BOOL_VEC2, 2, GLboolean);
		CASE(GL_BOOL_VEC3, 3, GLboolean);
		CASE(GL_BOOL_VEC4, 4, GLboolean);
		CASE(GL_FLOAT_MAT2, 4, GLfloat);
		CASE(GL_FLOAT_MAT2x3, 6, GLfloat);
		CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
		CASE(GL_FLOAT_MAT3, 9, GLfloat);
		CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
		CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
		CASE(GL_FLOAT_MAT4, 16, GLfloat);
		CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
		CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
#undef CASE
	default:
		std::cerr << "Unknown type: " << type << std::endl;
		exit(EXIT_FAILURE);
		break;
	}

	return size;
}

GLFWimage rgba_bmp_load(const char* filePath)
{
#define BMP_HEADER_SIZE 54
	GLFWimage image;
	image.width = 0;
	image.height = 0;
	image.pixels = nullptr;

	std::ifstream ifs(filePath);
	if (!ifs.is_open()) {
		std::cerr << "Unable to open file '" << filePath << "'" << std::endl;
		return image;
	}

	ifs.seekg(0, std::ios::end);
	size_t length = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	if (length < BMP_HEADER_SIZE) {
		std::cerr << "Bad BMP format!" << std::endl;
		return image;
	}

	char* buffer = new char[length];
	ifs.read(buffer, length);

	i32 data_position = static_cast<i32>(buffer[10]);
	i32 width = static_cast<i32>(buffer[18]);
	i32 height = static_cast<i32>(buffer[22]);

	if (data_position == 0) {
		data_position = BMP_HEADER_SIZE;
	}

	unsigned char* pixels = new unsigned char[length - data_position];
	memcpy(pixels, &buffer[data_position], length - data_position);

	image.width = width;
	image.height = height;
	image.pixels = pixels;

	return image;
#undef BMP_HEADER_SIZE
}

void Debug_Callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* user_param)
{
	std::string namedType;
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		namedType = "GL_DEBUG_TYPE_ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		namedType = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		namedType = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		namedType = "GL_DEBUG_TYPE_PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		namedType = "GL_DEBUG_TYPE_PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_MARKER:
		namedType = "GL_DEBUG_TYPE_MARKER";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		namedType = "GL_DEBUG_TYPE_PUSH_GROUP";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		namedType = "GL_DEBUG_TYPE_POP_GROUP";
		break;
	case GL_DEBUG_TYPE_OTHER:
		namedType = "GL_DEBUG_TYPE_OTHER";
		break;
	default:
		namedType = "UNDEFINED TYPE";
		break;
	}

	std::string namedSeverity;
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		namedSeverity = "GL_DEBUG_SEVERITY_HIGH";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		namedSeverity = "GL_DEBUG_SEVERITY_MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		namedSeverity = "GL_DEBUG_SEVERITY_LOW";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		namedSeverity = "GL_DEBUG_SEVERITY_NOTIFICATION";
#ifdef OPENGL_SUPRESS_NOTIFICATION
		return;
#endif //OPENGL_SUPRESS_NOTIFICATION
		break;
	default:
		namedSeverity = "UNDEFINED SEVERITY";
		break;
	}


	fprintf(stderr, "\nGL CALLBACK: %s type = %s\nSEVERITY = %s\nMESSAGE = %s\n\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		namedType.c_str(), namedSeverity.c_str(), message);
}