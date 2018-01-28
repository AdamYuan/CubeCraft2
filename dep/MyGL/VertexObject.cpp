#include "VertexObject.hpp"
namespace MyGL
{
	VertexObject::VertexObject(bool haveIndices) : HaveIndices(haveIndices), Elements(0), DataLength(0)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		if(HaveIndices)
			glGenBuffers(1, &EBO);
	}
	VertexObject::~VertexObject()
	{
		glDeleteBuffers(1, &VBO);
		if(HaveIndices)
			glDeleteBuffers(1, &EBO);
		glDeleteVertexArrays(1, &VAO);
	}
	void VertexObject::Render(GLenum mode)
	{
		if(Empty())
			return;

		glBindVertexArray(VAO);
		if(HaveIndices)
			glDrawElements(mode, Elements, GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(mode, 0, Elements);
		glBindVertexArray(0);
	}

	bool VertexObject::Empty() const
	{
		return Elements == 0;
	}

	void VertexObject::SetIndicesVec(const std::vector<unsigned int> &vec)
	{
		if(!HaveIndices)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, vec.size() * sizeof(unsigned int), &vec[0], GL_STATIC_DRAW);

		Elements = (unsigned int)vec.size();
	}

	void VertexObject::SetIndicesArr(const unsigned int *array, unsigned int arrLength)
	{
		if(!HaveIndices)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, arrLength * sizeof(unsigned int), array, GL_STATIC_DRAW);

		Elements = arrLength;
	}

	VertexObjectPtr NewVertexObject(bool haveIndices)
	{
		return std::make_unique<MyGL::VertexObject>(haveIndices);
	}
}
