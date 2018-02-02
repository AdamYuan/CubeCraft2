#ifndef TYPE_CPP
#define TYPE_CPP

#include <glm/glm.hpp>

enum Face { Right = 0, Left, Top, Bottom, Front, Back };

class AABB
{
public:
	glm::vec3 min_, max_;
	AABB(const glm::vec3 &_min, const glm::vec3 &_max);
	inline bool Intersect(const AABB &r) const
	{
		return (min_.x < r.max_.x && max_.x > r.min_.x) &&
			   (min_.y < r.max_.y && max_.y > r.min_.y) &&
			   (min_.z < r.max_.z && max_.z > r.min_.z);
	}
	inline bool Touch(const AABB &r, float delta) const
	{
		return (min_.x <= r.max_.x + delta && max_.x >= r.min_.x - delta) &&
			   (min_.y <= r.max_.y + delta && max_.y >= r.min_.y - delta) &&
			   (min_.z <= r.max_.z + delta && max_.z >= r.min_.z - delta);
	}
};

namespace util
{
	inline glm::ivec3 FaceExtend(glm::ivec3 pos, short face)
	{ pos[face>>1] += 1 - ((face&1)<<1); return pos; }
}

#endif
