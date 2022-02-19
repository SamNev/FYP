#include <GL/glew.h>
#include <string>

class ShaderProgram
{
public:
	ShaderProgram(const std::string vertexShader, const std::string fragmentShader);
	GLuint getId() { return m_id; }
	void use() { glUseProgram(m_id); }
	GLuint getUniform(std::string uniformName) { return glGetUniformLocation(m_id, uniformName.c_str()); }
private:
	GLuint m_id;
};