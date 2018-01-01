#ifndef TYPE_CPP
#define TYPE_CPP

#include <glm/vec3.hpp>

enum Face { Right = 0, Left, Top, Bottom, Front, Back };

class AABB
{
public:
	glm::vec3 Min, Max;
	AABB(const glm::vec3 &_min, const glm::vec3 &_max);

	bool Intersect(const AABB &r) const;
};

#endif
