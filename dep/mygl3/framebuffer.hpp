//
// Created by adamyuan on 4/15/18.
//

#ifndef MYGL2_FRAMEBUFFER_HPP
#define MYGL2_FRAMEBUFFER_HPP

#include <cassert>
#include <GL/gl3w.h>
#include "texture.hpp"

namespace mygl3
{
	class RenderBuffer
	{
	private:
		GLuint rbo_;
	public:
		explicit RenderBuffer() : rbo_(0) {}
		RenderBuffer (const RenderBuffer&) = delete;
		RenderBuffer& operator= (const RenderBuffer&) = delete;
		RenderBuffer(RenderBuffer &&buffer) noexcept : rbo_(buffer.rbo_) { buffer.rbo_ = 0; }
		RenderBuffer& operator= (RenderBuffer&& buffer) noexcept
		{
			assert(rbo_ == 0);
			rbo_ = buffer.rbo_; buffer.rbo_ = 0;
			return *this;
		}
		~RenderBuffer()
		{
			if(rbo_ != 0)
				glDeleteRenderbuffers(1, &rbo_);
		}
		void Initialize()
		{
			assert(rbo_ == 0);
			glCreateRenderbuffers(1, &rbo_);
		}
		void Load(GLenum internal_format, GLsizei width, GLsizei height)
		{
			assert(rbo_ != 0);
			glNamedRenderbufferStorage(rbo_, internal_format, width, height);
		}
		GLuint Get() const { return rbo_; }
	};

	class FrameBuffer
	{
	private:
		GLuint fbo_;
	public:
		explicit FrameBuffer() : fbo_(0) {}
		FrameBuffer (const FrameBuffer&) = delete;
		FrameBuffer& operator= (const FrameBuffer&) = delete;
		FrameBuffer(FrameBuffer &&buffer) noexcept : fbo_(buffer.fbo_) { buffer.fbo_ = 0; }
		FrameBuffer& operator= (FrameBuffer&& buffer) noexcept
		{
			assert(fbo_ == 0);
			fbo_ = buffer.fbo_; buffer.fbo_ = 0;
			return *this;
		}
		~FrameBuffer()
		{
			if(fbo_ != 0)
				glDeleteFramebuffers(1, &fbo_);
		}
		void Initialize()
		{
			assert(fbo_ == 0);
			glCreateFramebuffers(1, &fbo_);
		}
		void Bind()
		{
			assert(fbo_ != 0);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		}
		static void Unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		template<GLenum T> void AttachTexture(const Texture <T> &texture, GLenum attachment, GLint level = 0)
		{
			assert(fbo_ != 0);
			glNamedFramebufferTexture(fbo_, attachment, texture.Get(), level);
		}
		void AttachRenderbuffer(const RenderBuffer &rbo, GLenum attachment)
		{
			assert(fbo_ != 0);
			glNamedFramebufferRenderbuffer(fbo_, attachment, GL_RENDERBUFFER, rbo.Get());
		}
		GLuint Get() const { return fbo_; }
	};
}

#endif //MYGL2_FRAMEBUFFER_HPP
