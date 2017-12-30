#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace MyGL
{
	class Frustum
	{
	private:
		float planes[6][4];

	public:
		void CalculatePlanes(const glm::mat4 &mvp);

		bool PointInFrustum(float x, float y, float z) const;
		bool PointInFrustum(const glm::vec3 &pos) const;

		bool SphereInFrustum(float x, float y, float z, float rad) const;
		bool SphereInFrustum(const glm::vec3 &pos, float rad) const;

		bool CubeInFrustum(float x, float y, float z, float size) const;
		bool CubeInFrustum(const glm::vec3 &pos, float size) const;
	};
}
