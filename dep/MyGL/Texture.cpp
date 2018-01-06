#include "Texture.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace MyGL
{
	Texture::Texture()
	{
		glGenTextures(1, &id);
	}
	Texture::~Texture()
	{
		glDeleteTextures(1, &id);
	}
	void Texture::Load2d(const std::string &file)
	{
		textureType = GL_TEXTURE_2D;

		Bind();

		loadImageFile(file);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, imageData);

		freeImageArray();

		Unbind();
	}
	void Texture::Load2dArray(const std::string &file, int depth)
	{
		textureType = GL_TEXTURE_2D_ARRAY;

		Bind();

		loadImageFile(file);

		imageHeight /= depth;

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, imageWidth, imageHeight, depth, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, imageData);

		freeImageArray();

		Unbind();
	}
	void Texture::LoadCubemap(const std::vector<std::string> &faces)
	{
		textureType = GL_TEXTURE_CUBE_MAP;

		Bind();
		for(unsigned int i=0; i<faces.size(); i++)
		{
			loadImageFile(faces[i]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData
            );
			freeImageArray();
		}
		Unbind();
	}
	void Texture::BuildMipmap()
	{
		Bind();

		glGenerateMipmap(textureType);

		Unbind();
	}
	void Texture::SetParameters(GLenum min_filter, GLenum mag_filter, GLenum wrap_filter)
	{
		Bind();

		glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, mag_filter);
		glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrap_filter);
		glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrap_filter);

		if(textureType == GL_TEXTURE_CUBE_MAP)
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_filter);

		Unbind();
	}
	void Texture::Bind() const
	{
		glBindTexture(textureType, id);
	}
	void Texture::Unbind() const
	{
		glBindTexture(textureType, 0);
	}

	inline void Texture::loadImageFile(const std::string &file)
	{
		imageData = stbi_load(file.c_str(), &imageWidth, &imageHeight, &nrChannels, 4);
		if(!imageData)
			throw std::runtime_error("Failed to load image " + file);
	}

	void Texture::freeImageArray() {
		stbi_image_free(imageData);
	}

	GLuint Texture::GetId() const
	{
		return id;
	}


	TexturePtr NewTexture()
	{
		return std::make_unique<MyGL::Texture>();
	}
}
