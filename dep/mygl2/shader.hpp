//
// Created by adamyuan on 3/11/18.
//

#ifndef MYGL2_SHADER_HPP
#define MYGL2_SHADER_HPP

#include <glm/glm.hpp> //glm is great
#include <glm/gtc/type_ptr.hpp>

#include <GL/gl3w.h>
#include <fstream>
#include <cassert>

namespace mygl2
{
	class Shader
	{
	private:
		GLuint program_;
		std::string load_file(const char *filename) const
		{
			std::ifstream in(filename);
			if(!in.is_open())
			{
				printf("failed to open file %s\n", filename);
				return "";
			}
			return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{}};
		}

	public:
		Shader() : program_(0) {}

		~Shader()
		{
			if(program_ != 0)
				glDeleteProgram(program_);
		}
		Shader(const Shader&) = delete;
		Shader& operator= (const Shader&) = delete;
		Shader(Shader &&shader) noexcept : program_(shader.program_) { shader.program_ = 0; }
		Shader &operator=(Shader &&shader) noexcept
		{
			assert(program_ == 0);
			program_ = shader.program_;
			shader.program_ = 0;
			return *this;
		}

		void Initialize()
		{
			assert(program_ == 0);
			program_ = glCreateProgram();
		}

		void Load(const std::string &str, GLenum type)
		{
			const char *src = str.c_str();
			GLuint shader = glCreateShader(type);
			glShaderSource(shader, 1, &src, nullptr);
			glCompileShader(shader);

			int success;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if(!success)
			{
				char log[16384];
				glGetShaderInfoLog(shader, 16384, nullptr, log);
				printf("******SHADER COMPILE ERROR******\nsrc:\n%s\nerr:\n%s\n\n\n", src, log);
			}
			glAttachShader(program_, shader);
			glLinkProgram(program_);

			glDeleteShader(shader);
		}

		void LoadFromFile(const char *filename, GLenum type)
		{
			Load(load_file(filename), type);
		}

		void Use() const
		{
			assert(program_ != 0);
			glUseProgram(program_);
		}
		GLint GetUniform(const char *name) const { return glGetUniformLocation(program_, name); }
		GLuint GetProgram() const { return program_; }
		static void SetMat4(GLint loc, const glm::mat4 &matrix4) { glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix4)); }
		static void SetMat3(GLint loc, const glm::mat3 &matrix3) { glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(matrix3)); }
		static void SetVec4(GLint loc, const glm::vec4 &vector4) { glUniform4fv(loc, 1, glm::value_ptr(vector4)); }
		static void SetVec3(GLint loc, const glm::vec3 &vector3) { glUniform3fv(loc, 1, glm::value_ptr(vector3)); }
		static void SetInt(GLint loc, int i) { glUniform1i(loc, i); }
		static void SetUint(GLint loc, unsigned i) { glUniform1ui(loc, i); }
		static void SetFloat(GLint loc, float f) { glUniform1f(loc, f); }
	};

}

#endif //MYGL2_SHADER_HPP
