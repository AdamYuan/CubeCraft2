//
// Created by adamyuan on 4/15/18.
//

#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mygl3
{
	ImageLoader::ImageLoader(const char *filename)
	{
		info_.internal_format = GL_RGBA8;
		info_.format = GL_RGBA;
		info_.type = GL_UNSIGNED_BYTE;
		int nr_channels;
		info_.data = stbi_load(filename, &info_.width, &info_.height, &nr_channels, 4);
		assert(info_.data != nullptr);
	}
	ImageLoader::ImageLoader(const char *filename, GLsizei depth)
	{
		info_.internal_format = GL_RGBA8;
		info_.format = GL_RGBA;
		int nr_channels;
		info_.data = stbi_load(filename, &info_.width, &info_.height, &nr_channels, 4);
		info_.height /= depth;
		info_.depth = depth;
		assert(info_.data != nullptr);
	}
	ImageLoader::~ImageLoader() { stbi_image_free(info_.data); }
}