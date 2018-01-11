#ifndef TYPE_CPP
#define TYPE_CPP

#include <glm/glm.hpp>

enum Face { Right = 0, Left, Top, Bottom, Front, Back };

class AABB
{
public:
	glm::vec3 Min, Max;
	AABB(const glm::vec3 &_min, const glm::vec3 &_max);
	inline bool Intersect(const AABB &r) const
	{
		return (Min.x < r.Max.x && Max.x > r.Min.x) &&
			   (Min.y < r.Max.y && Max.y > r.Min.y) &&
			   (Min.z < r.Max.z && Max.z > r.Min.z);
	}
	inline bool Touch(const AABB &r, float delta) const
	{
		return (Min.x <= r.Max.x + delta && Max.x >= r.Min.x - delta) &&
			   (Min.y <= r.Max.y + delta && Max.y >= r.Min.y - delta) &&
			   (Min.z <= r.Max.z + delta && Max.z >= r.Min.z - delta);
	}
};

namespace Util
{
	inline glm::ivec3 FaceExtend(glm::ivec3 pos, short face)
	{ pos[face>>1] += 1 - ((face&1)<<1); return pos; }
}

#endif
