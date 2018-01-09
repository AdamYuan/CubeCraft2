//
// Created by adamyuan on 1/1/18.
//
#include "Util.hpp"

AABB::AABB(const glm::vec3 &_min, const glm::vec3 &_max) : Min(_min), Max(_max)
{

}

bool AABB::Intersect(const AABB &r) const
{
	return (Min.x < r.Max.x && Max.x > r.Min.x) &&
		   (Min.y < r.Max.y && Max.y > r.Min.y) &&
		   (Min.z < r.Max.z && Max.z > r.Min.z);
}

bool AABB::Touch(const AABB &r, float delta) const
{
	return (Min.x <= r.Max.x + delta && Max.x >= r.Min.x - delta) &&
		   (Min.y <= r.Max.y + delta && Max.y >= r.Min.y - delta) &&
		   (Min.z <= r.Max.z + delta && Max.z >= r.Min.z - delta);
}

float AABB::IntersectDistance(const AABB &r, int axis) const
{
	return glm::min(glm::abs(Max[axis] - r.Min[axis]), glm::abs(r.Max[axis] - Min[axis]));
}

