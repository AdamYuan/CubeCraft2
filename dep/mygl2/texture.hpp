//
// Created by adamyuan on 4/14/18.
//

#ifndef MYGL2_TEXTURE_HPP
#define MYGL2_TEXTURE_HPP

#include <GL/gl3w.h>
#include <cassert>

namespace mygl2
{
	struct ImageInfo
	{
		GLsizei width, height, depth;
		GLenum internal_format, format, type;
		GLvoid *data;
		ImageInfo()
				: width(1), height(1), depth(1), internal_format(GL_RGBA), format(GL_RGBA), type(GL_UNSIGNED_BYTE),
				  data(nullptr) {}
		ImageInfo(GLsizei width, GLsizei height, GLsizei depth, GLenum internal_format, GLenum format,
				  GLenum type, GLvoid *data)
				: width(width), height(height), depth(depth), internal_format(internal_format), format(format),
				  type(type), data(data) {}
	};

	class ImageLoader
	{
	private:
		ImageInfo info_;
	public:
		explicit ImageLoader(const char *filename);
		ImageLoader(const char *filename, GLsizei depth);
		const ImageInfo &GetInfo() const { return info_; }
		~ImageLoader();
	};

	template <GLenum T>
	class Texture
	{
	private:
		GLuint tex_;
	public:
		Texture() { tex_ = 0; }
		Texture& operator= (const Texture&) = delete;
		Texture(Texture &&texture) noexcept : tex_(texture.tex_)
		{
			texture.tex_ = 0;
		}
		~Texture()
		{
			if(tex_ != 0)
				glDeleteTextures(1, &tex_);
		}
		Texture (const Texture&) = delete;
		Texture& operator= (Texture &&texture) noexcept
		{
			assert(tex_ == 0);
			tex_ = texture.tex_;
			texture.tex_ = 0;
			return *this;
		}
#define IS_1D (T == GL_TEXTURE_1D || T == GL_PROXY_TEXTURE_1D)
#define IS_2D (T == GL_TEXTURE_2D || T == GL_PROXY_TEXTURE_2D || T == GL_TEXTURE_1D_ARRAY || \
	T == GL_PROXY_TEXTURE_1D_ARRAY || T == GL_TEXTURE_RECTANGLE || T == GL_PROXY_TEXTURE_RECTANGLE)
#define IS_3D (T == GL_TEXTURE_3D || T == GL_PROXY_TEXTURE_3D || T == GL_TEXTURE_2D_ARRAY || \
	T == GL_PROXY_TEXTURE_2D_ARRAY)

		void Initialize()
		{
			assert(tex_ == 0);
			glGenTextures(1, &tex_);
		}

		void Bind() const
		{
			assert(tex_ != 0);
			glBindTexture(T, tex_);
		}
		void Bind(GLenum slot) const
		{
			assert(tex_ != 0);
			glActiveTexture(slot);
			glBindTexture(T, tex_);
		}

		void Load(const ImageInfo &info)
		{
			Bind();
			if (IS_1D)
				glTexImage1D(T, 0, info.internal_format, info.width, 0, info.format, info.type, info.data);
			else if (IS_2D)
				glTexImage2D(T, 0, info.internal_format, info.width, info.height, 0, info.format, info.type, info.data);
			else if (IS_3D)
				glTexImage3D(T, 0, info.internal_format, info.width, info.height, info.depth, 0, info.format,
							 info.type, info.data);
		}

		void SetWrapFilter(GLenum filter)
		{
			Bind();
			if (IS_1D)
				glTexParameteri(T, GL_TEXTURE_WRAP_S, filter);
			else if (IS_2D)
			{
				glTexParameteri(T, GL_TEXTURE_WRAP_S, filter);
				glTexParameteri(T, GL_TEXTURE_WRAP_T, filter);
			}
			else if (IS_3D)
			{
				glTexParameteri(T, GL_TEXTURE_WRAP_S, filter);
				glTexParameteri(T, GL_TEXTURE_WRAP_T, filter);
				glTexParameteri(T, GL_TEXTURE_WRAP_R, filter);
			}
		}

		void SetSizeFilter(GLenum min_filter, GLenum mag_filter)
		{
			Bind();
			glTexParameteri(T, GL_TEXTURE_MIN_FILTER, min_filter);
			glTexParameteri(T, GL_TEXTURE_MAG_FILTER, mag_filter);
		}

		void GenerateMipmap()
		{
			Bind();
			glGenerateMipmap(T);
		}

		GLuint Get() const { return tex_; }
	};
	using Texture1D = Texture<GL_TEXTURE_1D>;
	using Texture1DArray = Texture<GL_TEXTURE_1D_ARRAY>;
	using Texture2D = Texture<GL_TEXTURE_2D>;
	using Texture2DArray = Texture<GL_TEXTURE_2D_ARRAY>;
	using Texture3D = Texture<GL_TEXTURE_3D>;
}

#endif //MYGL2_TEXTURE_HPP
