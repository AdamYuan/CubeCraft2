#include "Block.hpp"

namespace BlockMethods
{
	const BlockProperty BlockProperties[BLOCKS_NUM] =
			{
					{true,	true,	false,	0,	{}},					//Air
					{false,	false,	true,	0,	{0, 0, 0, 0, 0, 0}},	//Dirt
					{false,	false,	true,	0,	{1, 1, 2, 0, 1, 1}},	//Grass
					{false,	false,	true,	0,	{3, 3, 3, 3, 3, 3}},	//Stone
					{false,	false,	true,	0,	{4, 4, 4, 4, 4, 4}},	//Bedrock
					{false,	false,	true,	0,	{5, 5, 6, 6, 5, 5}},	//Wood
					{true,	false,	true,	0,	{7, 7, 7, 7, 7, 7}},	//Leaves
					{false,	false,	true,	15,	{8, 8, 8, 8, 8, 8}}		//Glowstone
			};

}
