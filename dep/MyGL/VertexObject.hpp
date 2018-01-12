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
		GLuint VAO, VBO;
		int Elements = 0, DataSize = 0;
		void bindAll();
		void unbindAll();

	public:
		VertexObject(const VertexObject &) = delete;
		VertexObject();
		~VertexObject();

		template<class T>
		void SetDataVec(std::vector<T> vec);

		template<class T>
		void SetDataArr(const T *array, int arrSize);

		void SetAttributes(int attr_count, ...);
		void Render(GLenum mode);

		bool Empty() const;
		size_t Size() const;
	};

	using VertexObjectPtr = std::unique_ptr<VertexObject>;


	template<class T>
	void VertexObject::SetDataVec(std::vector<T> vec)
	{
		if(vec.empty()) {
			DataSize = 0;
			return;
		}
		bindAll();
		glBufferData(GL_ARRAY_BUFFER, vec.size() * sizeof(T), &vec[0], GL_STATIC_DRAW);
		DataSize = (int) (vec.size() * (sizeof(T) / sizeof(float)));
		unbindAll();
	}
	template<class T>
	void VertexObject::SetDataArr(const T *array, int arrSize)
	{
		if(arrSize == 0) {
			DataSize = 0;
			return;
		}
		bindAll();
		glBufferData(GL_ARRAY_BUFFER, arrSize*sizeof(T), array, GL_STATIC_DRAW);
		DataSize = arrSize * (sizeof(T) / sizeof(float));
		unbindAll();
	}

	extern VertexObjectPtr NewVertexObject();
}
