#pragma once
#include <vector>
#include <iostream>
#include <memory>
#include <GL/glew.h>

namespace MyGL
{
	class VertexObject
	{
	private:
		GLuint VAO, VBO, EBO;
		unsigned Elements, DataLength;
		bool HaveIndices;

		unsigned getAttrSum() { return 0; }
		template<class... T>
		unsigned getAttrSum(unsigned attrId, unsigned attrSize, T... args) { return attrSize + getAttrSum(args...); }

		void setAttrImpl(unsigned, unsigned) { }
		template<class... T>
		void setAttrImpl(unsigned sum, unsigned current, unsigned attrId, unsigned attrSize, T... args)
		{
			glVertexAttribPointer(attrId, attrSize, GL_FLOAT, GL_FALSE,
								  sum * sizeof(float), (void*)(current * sizeof(float)));
			glEnableVertexAttribArray(attrId);
			setAttrImpl(sum, current + attrSize, args...);
		}

	public:
		VertexObject(const VertexObject &) = delete;
		explicit VertexObject(bool haveIndices);
		~VertexObject();

		template<class T>
		void SetDataVec(const std::vector<T> &vec);
		template<class T>
		void SetDataArr(const T *array, unsigned int arrLength);

		void SetIndicesVec(const std::vector<unsigned int> &vec);
		void SetIndicesArr(const unsigned int *array, unsigned int arrLength);

		template<class... T>
		void SetAttributes(T... args);

		void Render(GLenum mode);

		bool Empty() const;
	};

	using VertexObjectPtr = std::unique_ptr<VertexObject>;


	template<class T>
	void VertexObject::SetDataVec(const std::vector<T> &vec)
	{
		if(vec.empty()) {
			DataLength = 0;
			return;
		}
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vec.size() * sizeof(T), &vec[0], GL_STATIC_DRAW);

		if(!HaveIndices)
			DataLength = (unsigned)(vec.size() * (sizeof(T) / sizeof(float)));
	}
	template<class T>
	void VertexObject::SetDataArr(const T *array, unsigned int arrLength)
	{
		if(arrLength == 0) {
			DataLength = 0;
			return;
		}
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, arrLength * sizeof(T), array, GL_STATIC_DRAW);

		if(!HaveIndices)
			DataLength = arrLength * (sizeof(T) / sizeof(float));
	}

	template<class... T>
	void VertexObject::SetAttributes(T... args)
	{
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		unsigned sum = getAttrSum(args...);
		setAttrImpl(sum, 0, args...);

		if(!HaveIndices)
			Elements = DataLength / sum;
	}

	extern VertexObjectPtr NewVertexObject(bool haveIndices);
}
