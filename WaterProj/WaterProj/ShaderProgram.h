#include <GL/glew.h>
#include <string>

/***************************************************************************//**
 * Create a shader program for rendering, housed in the MapRenderer
 ******************************************************************************/
class ShaderProgram
{
public:
	/***************************************************************************//**
	 * Create a shader program from the given strings as GLSL code.
	 @param vertexShader Vertex shader as GLSL code
	 @pram fragmentShader Fragment shader as GLSL code
	 ******************************************************************************/
	ShaderProgram(const std::string vertexShader, const std::string fragmentShader);
	GLuint getId() { return m_id; }
	/***************************************************************************//**
	 * Use this shader program to render
	 ******************************************************************************/
	void use() { glUseProgram(m_id); }
	GLuint getUniform(std::string uniformName) { return glGetUniformLocation(m_id, uniformName.c_str()); }
private:
	GLuint m_id;
};