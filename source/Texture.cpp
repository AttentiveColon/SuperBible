#include "Texture.h"

#include "GL/glew.h"
#include <iostream>
#include <fstream>
#include <vector>



struct KTX_Header {
	unsigned char	identifier[12];
	unsigned int	endianness;
	unsigned int	gltype;
	unsigned int	gltypesize;
	unsigned int	glformat;
	unsigned int	glinternalformat;
	unsigned int	glbaseinternalformat;
	unsigned int	pixelwidth;
	unsigned int	pixelheight;
	unsigned int	pixeldepth;
	unsigned int	arrayelements;
	unsigned int	faces;
	unsigned int	miplevels;
	unsigned int	keypairbytes;
};

static const unsigned int swap32(const unsigned int value) {
	union
	{
		unsigned int value;
		unsigned char byte[4];
	} a, b;

	a.value = value;
	b.byte[0] = a.byte[3];
	b.byte[1] = a.byte[2];
	b.byte[2] = a.byte[1];
	b.byte[3] = a.byte[0];

	return b.value;
}

void SwapHeader(KTX_Header& header) {
	header.endianness = swap32(header.endianness);
	header.gltype = swap32(header.gltype);
	header.gltypesize = swap32(header.gltypesize);
	header.glformat = swap32(header.glformat);
	header.glinternalformat = swap32(header.glinternalformat);
	header.glbaseinternalformat = swap32(header.glbaseinternalformat);
	header.pixelwidth = swap32(header.pixelwidth);
	header.pixelheight = swap32(header.pixelheight);
	header.pixeldepth = swap32(header.pixeldepth);
	header.arrayelements = swap32(header.arrayelements);
	header.faces = swap32(header.faces);
	header.miplevels = swap32(header.miplevels);
	header.keypairbytes = swap32(header.keypairbytes);
}

static unsigned int calculate_stride(const KTX_Header& h, unsigned int width, unsigned int pad = 4)
{
	unsigned int channels = 0;

	switch (h.glbaseinternalformat)
	{
	case GL_RED:    channels = 1;
		break;
	case GL_RG:     channels = 2;
		break;
	case GL_BGR:
	case GL_RGB:    channels = 3;
		break;
	case GL_BGRA:
	case GL_RGBA:   channels = 4;
		break;
	}

	unsigned int stride = h.gltypesize * channels * width;

	stride = (stride + (pad - 1)) & ~(pad - 1);

	return stride;
}

static unsigned int calculate_face_size(const KTX_Header& h)
{
	unsigned int stride = calculate_stride(h, h.pixelwidth);

	return stride * h.pixelheight;
}

GLuint Load_KTX(const char* filename, GLuint texture) {
	//Open filestream and check if successful
	std::ifstream ifs(filename, std::ios_base::binary);
	if (!ifs.is_open()) {
		return 0;
	}

	//Get length of file
	ifs.seekg(0, ifs.end);
	usize length = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	//If file isn't empty, move data into buffer
	std::vector<char> buffer;
	if (!(length > 0)) {
		return 0;
	}
	buffer.resize(length);
	ifs.read(&buffer[0], length);

	//If filesize is larger than header size, load data into header struct
	KTX_Header* header;
	if (!(length > sizeof(KTX_Header))) {
		return 0;
	}
	header = (KTX_Header*)&buffer[0];

	//Check endianness and swap if neeeded
	if (header->endianness == 0x04030201) {}
	else if (header->endianness == 0x01020304)
	{
		SwapHeader(*header);
	}
	else { return 0; }

	//Guess the approriate target type
	GLenum target = GL_NONE;
	if (header->pixelheight == 0) {
		if (header->arrayelements == 0) target = GL_TEXTURE_1D;
		else target = GL_TEXTURE_1D_ARRAY;
	}
	else if (header->pixeldepth == 0) {
		if (header->arrayelements == 0) {
			if (header->faces == 0) target = GL_TEXTURE_2D;
			else target = GL_TEXTURE_CUBE_MAP;
		}
		else {
			if (header->faces == 0) target = GL_TEXTURE_2D_ARRAY;
			else target = GL_TEXTURE_CUBE_MAP_ARRAY;
		}
	}
	else target = GL_TEXTURE_3D;

	if (target == GL_NONE || header->pixelwidth == 0 || (header->pixelheight == 0 && header->pixeldepth == 0))
	{
		return 0;
	}

	//Create new texture name if one wasn't passed in and bind it
	GLuint tex = texture;
	if (tex == 0) {
		glCreateTextures(target, 1, &tex);
	}
	glBindTexture(target, tex);

	//Get the data location and copy data into memory
	usize data_start = sizeof(KTX_Header) + header->keypairbytes;
	usize data_end = buffer.size();
	unsigned char* data = new unsigned char[data_end - data_start];
	//+4 to get past the 4 bytes of data for buffer length
	memcpy(data, &buffer[data_start + 4], data_end - data_start);

	if (header->miplevels == 0) {
		header->miplevels = 1;
	}

	//Only support GL_TEXTURE_2D for now
	switch (target)
	{
	case GL_TEXTURE_2D:
		if (header->gltype == GL_NONE)
		{
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, header->glinternalformat, header->pixelwidth, header->pixelheight, 0, 420 * 380 / 2, data);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, header->miplevels, header->glinternalformat, header->pixelwidth, header->pixelheight);
			{
				unsigned char* ptr = data;
				unsigned int height = header->pixelheight;
				unsigned int width = header->pixelwidth;
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				
				for (unsigned int i = 0; i < header->miplevels; i++)
				{
					glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, width, height, header->glformat, header->gltype, ptr);
					ptr += height * calculate_stride(*header, width, 1);
					height >>= 1;
					width >>= 1;
					if (!height)
						height = 1;
					if (!width)
						width = 1;
				}
			}
		}
		break;
	case GL_TEXTURE_CUBE_MAP:
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, header->miplevels, header->glinternalformat, header->pixelwidth, header->pixelheight);
		// glTexSubImage3D(GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0, h.pixelwidth, h.pixelheight, h.faces, h.glformat, h.gltype, data);
		{
			unsigned int face_size = calculate_face_size(*header);
			for (unsigned int i = 0; i < header->faces; i++)
			{
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, header->pixelwidth, header->pixelheight, header->glformat, header->gltype, data + face_size * i);
			}
		}
		break;
	default:
		std::cerr << "Unsupported Texture Type" << std::endl;
		delete[] data;
		return 0;
	}

	//Generate mipmaps if texture doesn't already have them
	if (header->miplevels == 1) {
		glGenerateMipmap(target);
	}


	//Clean up and return texture name
	delete[] data;
	ifs.close();

	return tex;
}

KTX_Raw Get_KTX_Raw(const char* filename) {
	//Open filestream and check if successful
	std::ifstream ifs(filename, std::ios_base::binary);
	if (!ifs.is_open()) {
		return KTX_Raw();
	}

	//Get length of file
	ifs.seekg(0, ifs.end);
	usize length = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	//If file isn't empty, move data into buffer
	std::vector<char> buffer;
	if (!(length > 0)) {
		return KTX_Raw();
	}
	buffer.resize(length);
	ifs.read(&buffer[0], length);

	//If filesize is larger than header size, load data into header struct
	KTX_Header* header;
	if (!(length > sizeof(KTX_Header))) {
		return KTX_Raw();
	}
	header = (KTX_Header*)&buffer[0];

	//Check endianness and swap if neeeded
	if (header->endianness == 0x04030201) {}
	else if (header->endianness == 0x01020304)
	{
		SwapHeader(*header);
	}
	else { return KTX_Raw(); }

	//Get the data location and copy data into memory
	usize data_start = sizeof(KTX_Header) + header->keypairbytes;
	usize data_end = buffer.size();
	unsigned char* data = new unsigned char[data_end - data_start];
	//+4 to get past the 4 bytes of data for buffer length
	memcpy(data, &buffer[data_start + 4], data_end - data_start);

	if (header->miplevels == 0) {
		header->miplevels = 1;
	}

	return KTX_Raw(
		data, 
		header->pixelwidth, 
		header->pixelheight, 
		header->glformat, 
		header->glinternalformat, 
		header->gltype,
		header->miplevels
	);
}

GLuint CreateTextureArray(const char* filenames[], size_t length)
{
	KTX_Raw temp = Get_KTX_Raw(filenames[0]);

	GLuint tex;
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &tex);
	glTextureStorage3D(tex, 1, temp.m_glinternal_format, temp.m_width, temp.m_height, length);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);

	

	int miplevels = temp.m_miplevels;

	int width = -1;
	int height = -1;
	for (size_t i = 0; i < length; ++i) {
		KTX_Raw texture = Get_KTX_Raw(filenames[i]);
		if (texture.m_data == nullptr) {
			std::cerr << "Couldn't load " << filenames[i] << "\nNo texture data!" << std::endl;
			return 0;
		}
		if (width == -1) { width = texture.m_width; }
		else if (width != texture.m_width) {
			std::cerr << "Couldn't create array! Not all files have same width dimensions" << std::endl;
			return 0;
		}
		if (height == -1) { height = texture.m_height; }
		else if (width != texture.m_height) {
			std::cerr << "Couldn't create array! Not all files have same height dimensions" << std::endl;
			return 0;
		}
		glTextureSubImage3D(tex, 0, 0, 0, i, texture.m_width, texture.m_height, 1, texture.m_glformat, texture.m_gltype, texture.m_data);
		
	}
	if (miplevels == 1) {
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return tex;
}
