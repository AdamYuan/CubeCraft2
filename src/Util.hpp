#ifndef TYPE_CPP
#define TYPE_CPP

#include <glm/glm.hpp>

enum Face { Right = 0, Left, Top, Bottom, Front, Back };

class AABB
{
public:
	glm::vec3 Min, Max;
	AABB(const glm::vec3 &_min, const glm::vec3 &_max);

	bool Intersect(const AABB &r) const;
	bool Touch(const AABB &r, float delta) const;
	float IntersectDistance(const AABB &r, int axis) const;
};

#endif
