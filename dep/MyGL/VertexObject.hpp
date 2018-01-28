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
		VertexObject();
		~VertexObject();

		template<class T>
		void SetDataVec(std::vector<T> vec);

		template<class T>
		void SetDataArr(const T *array, int arrSize);

		template<class... T>
		void SetAttributes(T... args)
		{
			bindAll();
			unsigned sum = getAttrSum(args...);
			setAttrImpl(sum, 0, args...);
			unbindAll();
			Elements = DataSize / sum;
		}

		void Render(GLenum mode);

		bool Empty() const;
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
