#pragma once
#include <string>
#include <GL/glew.h>
#include <vector>
#include <memory>

namespace MyGL
{
	class Texture
	{
	private:
		GLuint id;
		GLenum textureType;
		int imageWidth, imageHeight, nrChannels;
		unsigned char *imageData;
		void loadImageFile(const std::string &file);
		void freeImageArray();
	public:
		void Load2d(const std::string &file);
		void Load2dArray(const std::string &file, int depth);
		void LoadCubemap(const std::vector<std::string> &faces);
		void SetParameters(GLenum min_filter, GLenum mag_filter, GLenum wrap_filter);
		void BuildMipmap();
		void Bind() const;
		void Unbind() const;
		GLuint GetId() const;
		~Texture();
		Texture();
		Texture(const Texture &) = delete;
	};
	using TexturePtr = std::unique_ptr<Texture>;

	extern TexturePtr NewTexture();
}
