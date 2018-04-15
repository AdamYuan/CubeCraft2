//
// Created by adamyuan on 3/10/18.
//

#ifndef MYGL2_VERTEXBUFFER_HPP
#define MYGL2_VERTEXBUFFER_HPP

#include <GL/gl3w.h>
#include <vector>
#include <cassert>

namespace mygl2
{
	class VertexBuffer
	{
	private:
		GLuint vbo_, vao_, ebo_;
		GLsizei array_size_, attr_length_, elements_;
		void bind_vbo() const
		{
			assert(vbo_ != 0);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		}
		void bind_vao() const
		{
			glBindVertexArray(vao_);
		}
		void bind_ebo() const
		{
			assert(ebo_ != 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		}
		GLsizei get_attr_length() const { return 0; }
		template <class... T> GLsizei get_attr_length(GLuint, GLsizei attr_size, T... args) const { return attr_size + get_attr_length(args...); }

		void set_attr_impl(GLsizei, GLsizei) {}
		template <class... T> void set_attr_impl(GLsizei attr_length, GLsizei current, GLuint attr_id, GLsizei attr_size, T... args)
		{
			glVertexAttribPointer(attr_id, attr_size, GL_FLOAT, GL_FALSE, attr_length * sizeof(GLfloat),
								  (void*)(current * sizeof(GLfloat)));
			glEnableVertexAttribArray(attr_id);
			set_attr_impl(attr_length, current + attr_size, args...);
		}
	public:
		VertexBuffer() : vbo_(0), vao_(0), ebo_(0), array_size_(0), attr_length_(0), elements_(0) {  }
		VertexBuffer(VertexBuffer &&buffer) noexcept : vbo_(buffer.vbo_), vao_(buffer.vao_), ebo_(buffer.ebo_),
															array_size_(buffer.array_size_), attr_length_(buffer.attr_length_),
															elements_(buffer.elements_)
		{
			buffer.vbo_ = buffer.vao_ = buffer.ebo_ = 0;
			buffer.array_size_ = buffer.attr_length_ = buffer.elements_ = 0;
		}
		~VertexBuffer()
		{
			if(vbo_ != 0) //initialized
			{
				glDeleteBuffers(1, &vbo_);
				glDeleteBuffers(1, &ebo_);
				glDeleteVertexArrays(1, &vao_);
			}
		}
		VertexBuffer (const VertexBuffer&) = delete;
		VertexBuffer& operator= (const VertexBuffer&) = delete;
		VertexBuffer &operator=(VertexBuffer &&buffer) noexcept
		{
			vbo_ = buffer.vbo_; vao_ = buffer.vao_; ebo_ = buffer.ebo_; array_size_ = buffer.array_size_;
			attr_length_ = buffer.attr_length_; elements_ = buffer.elements_;
			buffer.vbo_ = buffer.vao_ = buffer.ebo_ = 0;
			buffer.array_size_ = buffer.attr_length_ = buffer.elements_ = 0;
			return *this;
		}
		void Initialize()
		{
			assert(vbo_ == 0);
			glGenBuffers(1, &vbo_);
			glGenBuffers(1, &ebo_);
			glGenVertexArrays(1, &vao_);
		}

		template<class T>
		void SetData(const T *array, GLsizei size, GLenum data_type)
		{
			bind_vao(); bind_vbo();
			glBufferData(GL_ARRAY_BUFFER, size * sizeof(T), array, data_type);
			array_size_ = size;

			if(elements_ == 0 && attr_length_ != 0)
			{
				assert(array_size_ % attr_length_ == 0);
				elements_ = array_size_ / attr_length_;
			}
		}
		template<class T>
		void SetData(const std::vector<T> &vec, GLenum data_type)
		{
			SetData(&vec[0], (GLsizei)vec.size(), data_type);
		}

		template<class... T>
		void SetAttributes(T... args)
		{
			bind_vao(); bind_vbo();
			attr_length_ = get_attr_length(args...);
			set_attr_impl(attr_length_, 0, args...);

			if(elements_ == 0 && array_size_ != 0)
			{
				assert(array_size_ % attr_length_ == 0);
				elements_ = array_size_ / attr_length_;
			}
		}

		void SetIndices(const GLuint *array, GLsizei size, GLenum data_type)
		{
			bind_vao(); bind_ebo();
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint), array, data_type);

			elements_ = size;
		}

		void SetIndices(const std::vector<GLuint> &vec, GLenum data_type)
		{
			SetIndices(&vec[0], (GLsizei)vec.size(), data_type);
		}

		void Render(GLenum type) const
		{
			bind_vao();
			glDrawArrays(type, 0, elements_);
		}

		void RenderWithIndices(GLenum type) const
		{
			bind_vao();
			glDrawElements(type, elements_, GL_UNSIGNED_INT, nullptr);
		}
		GLsizei Element() const { return elements_; }
		bool Empty() const { return elements_ == 0; }
	};

}

#endif //MYGL2_VERTEXBUFFER_HPP
