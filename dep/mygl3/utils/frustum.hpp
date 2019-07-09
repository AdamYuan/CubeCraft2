//
// Created by adamyuan on 4/15/18.
//

#ifndef MYGL2_FRUSTUM_HPP
#define MYGL2_FRUSTUM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mygl3
{
	class Frustum
	{
	private:
		float planes[6][4];

	public:

		//http://www.cnblogs.com/dragon2012/p/3891519.html
		void CalculatePlanes(const glm::mat4 &mvp_matrix)
		{
			//
			// Extract the frustum's right clipping plane and normalize it.
			//
			float t;

			const float *mvp = glm::value_ptr(mvp_matrix);

			planes[0][0] = mvp[ 3] - mvp[ 0];
			planes[0][1] = mvp[ 7] - mvp[ 4];
			planes[0][2] = mvp[11] - mvp[ 8];
			planes[0][3] = mvp[15] - mvp[12];

			t = glm::inversesqrt( planes[0][0] * planes[0][0] +
								  planes[0][1] * planes[0][1] +
								  planes[0][2] * planes[0][2] );

			planes[0][0] *= t;
			planes[0][1] *= t;
			planes[0][2] *= t;
			planes[0][3] *= t;

			//
			// Extract the frustum's left clipping plane and normalize it.
			//

			planes[1][0] = mvp[ 3] + mvp[ 0];
			planes[1][1] = mvp[ 7] + mvp[ 4];
			planes[1][2] = mvp[11] + mvp[ 8];
			planes[1][3] = mvp[15] + mvp[12];

			t = glm::inversesqrt( planes[1][0] * planes[1][0] +
								  planes[1][1] * planes[1][1] +
								  planes[1][2] * planes[1][2] );

			planes[1][0] *= t;
			planes[1][1] *= t;
			planes[1][2] *= t;
			planes[1][3] *= t;



			//
			// Extract the frustum's bottom clipping plane and normalize it.
			//

			planes[2][0] = mvp[ 3] + mvp[ 1];
			planes[2][1] = mvp[ 7] + mvp[ 5];
			planes[2][2] = mvp[11] + mvp[ 9];
			planes[2][3] = mvp[15] + mvp[13];

			t = glm::inversesqrt( planes[2][0] * planes[2][0] +
								  planes[2][1] * planes[2][1] +
								  planes[2][2] * planes[2][2] );

			planes[2][0] *= t;
			planes[2][1] *= t;
			planes[2][2] *= t;
			planes[2][3] *= t;

			//
			// Extract the frustum's top clipping plane and normalize it.
			//

			planes[3][0] = mvp[ 3] - mvp[ 1];
			planes[3][1] = mvp[ 7] - mvp[ 5];
			planes[3][2] = mvp[11] - mvp[ 9];
			planes[3][3] = mvp[15] - mvp[13];

			t = glm::inversesqrt( planes[3][0] * planes[3][0] +
								  planes[3][1] * planes[3][1] +
								  planes[3][2] * planes[3][2] );

			planes[3][0] *= t;
			planes[3][1] *= t;
			planes[3][2] *= t;
			planes[3][3] *= t;



			//
			// Extract the frustum's far clipping plane and normalize it.
			//

			planes[4][0] = mvp[ 3] - mvp[ 2];
			planes[4][1] = mvp[ 7] - mvp[ 6];
			planes[4][2] = mvp[11] - mvp[10];
			planes[4][3] = mvp[15] - mvp[14];

			t = glm::inversesqrt( planes[4][0] * planes[4][0] +
								  planes[4][1] * planes[4][1] +
								  planes[4][2] * planes[4][2] );

			planes[4][0] *= t;
			planes[4][1] *= t;
			planes[4][2] *= t;
			planes[4][3] *= t;

			//
			// Extract the frustum's near clipping plane and normalize it.
			//

			planes[5][0] = mvp[ 3] + mvp[ 2];
			planes[5][1] = mvp[ 7] + mvp[ 6];
			planes[5][2] = mvp[11] + mvp[10];
			planes[5][3] = mvp[15] + mvp[14];

			t = glm::inversesqrt( planes[5][0] * planes[5][0] +
								  planes[5][1] * planes[5][1] +
								  planes[5][2] * planes[5][2] );

			planes[5][0] *= t;
			planes[5][1] *= t;
			planes[5][2] *= t;
			planes[5][3] *= t;
		}

		//http://www.crownandcutlass.com/features/technicaldetails/frustum.html
		bool PointInFrustum(float x, float y, float z) const
		{
			int p;

			for( p = 0; p < 6; p++ )
				if(planes[p][0] * x + planes[p][1] * y + planes[p][2] * z + planes[p][3] <= 0 )
					return false;
			return true;
		}
		bool PointInFrustum(const glm::vec3 &pos) const
		{
			return PointInFrustum(pos.x, pos.y, pos.z);
		}

		bool SphereInFrustum(float x, float y, float z, float rad) const
		{
			int p;
			float d;

			for( p = 0; p < 6; p++ )
			{
				d = planes[p][0] * x + planes[p][1] * y + planes[p][2] * z + planes[p][3];
				if( d <= -rad )
					return 0;
			}
			return d + rad;
		}
		bool SphereInFrustum(const glm::vec3 &pos, float rad) const
		{
			return SphereInFrustum(pos.x, pos.y, pos.z, rad);
		}

		bool CubeInFrustum(float x, float y, float z, float size) const
		{
			int p;
			int c;
			int c2 = 0;

			for( p = 0; p < 6; p++ )
			{
				c = 0;
				if( planes[p][0] * (x - size) + planes[p][1] * (y - size) + planes[p][2] * (z - size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x + size) + planes[p][1] * (y - size) + planes[p][2] * (z - size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x - size) + planes[p][1] * (y + size) + planes[p][2] * (z - size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x + size) + planes[p][1] * (y + size) + planes[p][2] * (z - size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x - size) + planes[p][1] * (y - size) + planes[p][2] * (z + size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x + size) + planes[p][1] * (y - size) + planes[p][2] * (z + size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x - size) + planes[p][1] * (y + size) + planes[p][2] * (z + size) + planes[p][3] > 0 )
					c++;
				if( planes[p][0] * (x + size) + planes[p][1] * (y + size) + planes[p][2] * (z + size) + planes[p][3] > 0 )
					c++;
				if( c == 0 )
					return 0;
				if( c == 8 )
					c2++;
			}
			return (c2 == 6) ? 2 : 1;
		}

		bool CubeInFrustum(const glm::vec3 &pos, float size) const
		{
			return CubeInFrustum(pos.x, pos.y, pos.z, size);
		}
	};
}

#endif //MYGL2_FRUSTUM_HPP
