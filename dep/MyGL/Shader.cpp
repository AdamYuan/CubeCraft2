#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
namespace MyGL
{
	Shader::Shader()
	{
		programId = glCreateProgram();
	}
	Shader::~Shader()
	{
		glDeleteProgram(programId);
	}
	std::string Shader::loadFile(const std::string &file)
	{
		std::ifstream in(file);
		std::string str;
		if(!in.is_open())
			std::cout << "failed to Load Shader file: " << file << std::endl;
		else
			std::getline(in, str, '\0');
		return str;
	}
	void Shader::Load(const std::string &source, unsigned int mode)
	{
		GLuint shaderId = glCreateShader(mode);
		const char *csource = source.c_str();
		glShaderSource(shaderId, 1, &csource, nullptr);
		glCompileShader(shaderId);
		std::cout << "Loaded LightShader: \n" << source << std::endl;
		int success;
		char error[1000];
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(shaderId, 1000, nullptr, error);
			std::cout << "LightShader compile status: \n" << error << std::endl;
		}
		glAttachShader(programId,shaderId);
		glLinkProgram(programId);

		glDeleteShader(shaderId);
	}
	void Shader::LoadFromFile(const std::string &file, unsigned int mode)
	{
		std::string source = loadFile(file);
		Load(source, mode);
	}
	GLint Shader::GetUniform(const std::string &str) const
	{
		return glGetUniformLocation(programId, str.c_str());
	}
	GLint Shader::GetAttribute(const std::string &str) const
	{
		return glGetAttribLocation(programId, str.c_str());
	}
	void Shader::Use()
	{
		glUseProgram(programId);
	}


	ShaderPtr NewShader()
	{
		return std::make_unique<MyGL::Shader>();
	}
}
