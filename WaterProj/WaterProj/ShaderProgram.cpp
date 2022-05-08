#include "ShaderProgram.h"

#include <vector>

//TODO: does this need any changes? Probs not?

ShaderProgram::ShaderProgram(const std::string vertexShader, const std::string fragmentShader)
{
	const char* vert = vertexShader.c_str();
	const char* frag = fragmentShader.c_str();

	// Create a new vertex shader, attach source code, compile it and
	// check for errors.
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, &vert, NULL);
	glCompileShader(vertexShaderId);
	GLint success = 0;
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		GLint maxLength = 0;
		glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(vertexShaderId, maxLength, &maxLength, &errorLog[0]);
		std::string error = "Vertex shader failed to compile: ";
		for (size_t i = 0; i < maxLength; i++)
		{
			error.push_back(errorLog[i]);
		}
		throw std::exception(error.c_str());
	}

	// Create a new fragment shader, attach source code, compile it and
	// check for errors.
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, &frag, NULL);
	glCompileShader(fragmentShaderId);
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &maxLength);
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(fragmentShaderId, maxLength, &maxLength, &errorLog[0]);
		std::string error = "Fragment shader failed to compile: ";
		for (size_t i = 0; i < maxLength; i++)
		{
			error.push_back(errorLog[i]);
		}
		throw std::exception(error.c_str());
	}

	// Create new shader program and attach our shader objects
	m_id = glCreateProgram();
	glAttachShader(m_id, vertexShaderId);
	glAttachShader(m_id, fragmentShaderId);

	glBindAttribLocation(m_id, 0, "in_Position");

	glLinkProgram(m_id);
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);

	if (!success)
	{
		throw std::exception("Shader link failed");

	}
	// Detach and destroy the shader objects
	glDetachShader(m_id, vertexShaderId);
	glDeleteShader(vertexShaderId);
	glDetachShader(m_id, fragmentShaderId);
	glDeleteShader(fragmentShaderId);
}
