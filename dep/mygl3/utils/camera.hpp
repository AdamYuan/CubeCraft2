//
// Created by adamyuan on 4/15/18.
//

#ifndef MYGL2_CAMERA_HPP
#define MYGL2_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace mygl3
{
	class Camera
	{
	private:
		float yaw_, pitch_;
		glm::vec3 position_;
	public:
		float &Yaw() { return yaw_; }
		float &Pitch() { return pitch_; }
		glm::vec3 &Position() { return position_; }
		float GetYaw() const { return yaw_; }
		float GetPitch() const { return pitch_; }
		const glm::vec3 &GetPosition() const { return position_; }
		glm::mat4 GetMatrix() const
		{
			glm::mat4 view;
			view = glm::rotate(view, glm::radians(-pitch_), glm::vec3(1.0f, 0.0f, 0.0f));
			view = glm::rotate(view, glm::radians(-yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
			view = glm::translate(view, -position_);

			return view;
		}
		glm::vec3 GetLookDirection() const
		{
			glm::mat4 matrix;
			matrix = glm::rotate(matrix, glm::radians(yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
			matrix = glm::rotate(matrix, glm::radians(pitch_), glm::vec3(1.0f, 0.0f, 0.0f));
			return glm::mat3(matrix) * glm::vec3(0.0f, 0.0f, -1.0f);
		}

		void MoveForward(float dist, float dir) //degrees
		{
			float rad = glm::radians(yaw_ + dir);
			position_.x -= glm::sin(rad) * dist;
			position_.z -= glm::cos(rad) * dist;
		}

		void MoveUp(float dist)
		{
			position_.y += dist;
		}

		void MouseControl(float x_offset, float y_offset, float sensitivity)
		{
			x_offset *= sensitivity; y_offset *= sensitivity;
			yaw_ += x_offset; pitch_ += y_offset;
			pitch_ = glm::clamp(pitch_, -90.0f, 90.0f);
		}
	};
}

#endif //MYGL2_CAMERA_HPP
