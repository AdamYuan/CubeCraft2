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
