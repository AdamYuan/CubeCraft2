//
// Created by adamyuan on 4/15/18.
//

#ifndef MYGL2_FRAMERATE_HPP
#define MYGL2_FRAMERATE_HPP

#include <GLFW/glfw3.h>

namespace mygl3
{
	class Framerate
	{
	private:
		float delta_, fps_;
	public:
		Framerate() : delta_(0), fps_(0) {}
		void Update()
		{
			static float last_time{0};
			float now{(float)glfwGetTime()};
			delta_ = now - last_time;
			last_time = now;

		}
		float GetFps()
		{
			static float delta_sum{0}, last_get_time_{0};
			static unsigned times = 0;

			delta_sum += delta_;
			times ++;

			float now{(float)glfwGetTime()};
			if(now > last_get_time_ + 1.0f && times)
			{
				fps_ = (float)times / delta_sum;
				last_get_time_ = now;
				delta_sum = 0;
				times = 0;
			}

			return fps_;
		}
		float GetDelta() const { return delta_; }
	};
}

#endif //MYGL2_FRAMERATE_HPP
