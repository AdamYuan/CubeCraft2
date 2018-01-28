#include "VertexObject.hpp"
namespace MyGL
{
	VertexObject::VertexObject()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
	}
	VertexObject::~VertexObject()
	{
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}
	void VertexObject::bindAll()
	{
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
	}
	void VertexObject::unbindAll()
	{
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray(0);
	}
	void VertexObject::Render(GLenum mode)
	{
		if(Empty())
			return;
		glBindVertexArray(VAO);
		glDrawArrays(mode, 0, Elements);
		glBindVertexArray(0);
	}

	bool VertexObject::Empty() const
	{
		return Elements == 0;
	}

	VertexObjectPtr NewVertexObject()
	{
		return std::make_unique<MyGL::VertexObject>();
	}
}
