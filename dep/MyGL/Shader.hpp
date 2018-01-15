#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
namespace MyGL
{
	class Shader
	{
	private:
		std::string loadFile(const std::string &file);
		GLuint programId;
	public:
		explicit Shader();
		~Shader();
		Shader(const Shader &) = delete;
		void LoadFromFile(const std::string &file, unsigned int mode);
		void Load(const std::string &source, unsigned int mode);
		GLint GetUniform(const std::string &str) const;
		GLint GetAttribute(const std::string &str) const;
		inline void PassMat4(GLint loc, const glm::mat4 &matrix4)
		{ glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix4)); }
		inline void PassMat3(GLint loc, const glm::mat3 &matrix3)
		{ glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(matrix3)); }
		inline void PassVec4(GLint loc, const glm::vec4 &vector4)
		{ glUniform4fv(loc, 1, glm::value_ptr(vector4)); }
		inline void PassVec3(GLint loc, const glm::vec3 &vector3)
		{ glUniform3fv(loc, 1, glm::value_ptr(vector3)); }
		inline void PassInt(GLint loc, int i)
		{ glUniform1i(loc, i); }
		inline void PassFloat(GLint loc, float f)
		{ glUniform1f(loc, f); }
		void Use();
	};

	using ShaderPtr = std::unique_ptr<Shader>;

	extern ShaderPtr NewShader();
}
