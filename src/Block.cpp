#include "Block.hpp"

namespace BlockMethods
{
	const BlockProperty BlockProperties[BLOCKS_NUM] =
			{
					{"",			true,	true,	false,	0,	{}},
					{"Dirt",		false,	false,	true,	0,	{0, 0, 0, 0, 0, 0}},
					{"Grass",		false,	false,	true,	0,	{1, 1, 2, 0, 1, 1}},
					{"Stone",		false,	false,	true,	0,	{3, 3, 3, 3, 3, 3}},
					{"Bedrock",		false,	false,	true,	0,	{4, 4, 4, 4, 4, 4}},
					{"Wood",		false,	false,	true,	0,	{5, 5, 6, 6, 5, 5}},
					{"Leaves",		true,	false,	true,	0,	{7, 7, 7, 7, 7, 7}},
					{"Glowstone",	false,	false,	true,	15,	{8, 8, 8, 8, 8, 8}},
					{"Plank",		false,	false,	true,	0,	{9, 9, 9, 9, 9, 9}},
					{"Glass",		true,	true,	true,	0,	{10, 10, 10, 10, 10, 10}}
			};
}
