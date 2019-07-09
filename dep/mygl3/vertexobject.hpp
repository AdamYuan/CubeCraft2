//
// Created by adamyuan on 3/10/18.
//

#ifndef MYGL3_VERTEXBUFFER_HPP
#define MYGL3_VERTEXBUFFER_HPP

#include <GL/gl3w.h>
#include <vector>
#include <cassert>

namespace mygl3
{
	template <bool USE_INDEX>
	class VertexObject
	{
	private:
		GLuint vbo_, vao_, ebo_;
		GLsizei array_size_, attr_length_, elements_;
		GLsizei get_attr_length() const { return 0; }
		template <class... T> GLsizei get_attr_length(GLuint, GLsizei attr_size, T... args) const { return attr_size + get_attr_length(args...); }

		void set_attr_impl(GLsizei) {}
		template <class... T> void set_attr_impl(GLsizei current, GLuint attr_id, GLsizei attr_size, T... args)
		{
			glEnableVertexArrayAttrib(vao_, attr_id);
			glVertexArrayAttribFormat(vao_, attr_id, attr_size, GL_FLOAT, GL_FALSE, current * sizeof(GLfloat));
			glVertexArrayAttribBinding(vao_, attr_id, 0);
			set_attr_impl(current + attr_size, args...);
		}
	public:
		VertexObject() : vbo_(0), vao_(0), ebo_(0), array_size_(0), attr_length_(0), elements_(0) {  }
		VertexObject(VertexObject &&buffer) noexcept : vbo_(buffer.vbo_), vao_(buffer.vao_), ebo_(buffer.ebo_),
															array_size_(buffer.array_size_), attr_length_(buffer.attr_length_),
															elements_(buffer.elements_)
		{
			buffer.vbo_ = buffer.vao_ = buffer.ebo_ = 0;
			buffer.array_size_ = buffer.attr_length_ = buffer.elements_ = 0;
		}
		~VertexObject()
		{
			if(vbo_ != 0) //initialized
			{
				glDeleteBuffers(1, &vbo_);
				if(USE_INDEX) glDeleteBuffers(1, &ebo_);
				glDeleteVertexArrays(1, &vao_);
			}
		}
		VertexObject (const VertexObject&) = delete;
		VertexObject& operator= (const VertexObject&) = delete;
		VertexObject &operator=(VertexObject &&buffer) noexcept
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
			glCreateVertexArrays(1, &vao_);
			glCreateBuffers(1, &vbo_);
			if(USE_INDEX)
			{
				glCreateBuffers(1, &ebo_);
				glVertexArrayElementBuffer(vao_, ebo_);
			}
		}

		template<class T>
		void SetData(const T *array, GLsizei size, GLenum data_type)
		{
			glNamedBufferData(vbo_, size * sizeof(T), array, data_type);
			array_size_ = size;

			if(!USE_INDEX && attr_length_ != 0)
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
			attr_length_ = get_attr_length(args...);
			set_attr_impl(0, args...);
			glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, attr_length_ * sizeof(GLfloat));

			if(!USE_INDEX && array_size_ != 0)
			{
				assert(array_size_ % attr_length_ == 0);
				elements_ = array_size_ / attr_length_;
			}
		}

		void SetIndices(const GLuint *array, GLsizei size, GLenum data_type)
		{
			assert(USE_INDEX);
			glNamedBufferData(ebo_, size * sizeof(GLuint), array, data_type);
			elements_ = size;
			array_size_ = 0;
		}

		void SetIndices(const std::vector<GLuint> &vec, GLenum data_type)
		{
			SetIndices(&vec[0], (GLsizei)vec.size(), data_type);
		}

		void Render(GLenum type) const
		{
			assert(vbo_ != 0);
			glBindVertexArray(vao_);
			if(USE_INDEX)
				glDrawElements(type, elements_, GL_UNSIGNED_INT, nullptr);
			else
				glDrawArrays(type, 0, elements_);
		}
		GLsizei Element() const { return elements_; }
		bool Empty() const { return elements_ == 0; }
	};
}

#endif
