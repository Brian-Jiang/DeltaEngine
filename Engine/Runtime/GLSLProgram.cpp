#include "GLSLProgram.h"

#include <fstream>
#include <vector>

GLSLProgram::GLSLProgram(): programID(0), vertexShaderID(0), fragmentShaderID(0)
{
}

void GLSLProgram::compileShaders(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath)
{
	// Create a program object and store the ID
	programID = glCreateProgram();

	// Create the vertex shader object, and store its ID
	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	if (vertexShaderID == 0) {
		// Error creating shader
	}

	// Create the fragment shader object, and store its ID
	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if (fragmentShaderID == 0) {
		// Error creating shader
	}

	std::ifstream vertexFile(vertexShaderFilePath);
	if (vertexFile.fail()) {
		perror(vertexShaderFilePath.c_str());
	}

	std::ifstream fragmentFile(fragmentShaderFilePath);
	if (fragmentFile.fail()) {
		perror(fragmentShaderFilePath.c_str());
	}

	std::string fileContents = "";
	std::string line;
	while (std::getline(vertexFile, line)) {
		fileContents += line + "\n";
	}

	auto contents = fileContents.c_str();
	glShaderSource(vertexShaderID, 1, &contents, nullptr);
	glCompileShader(vertexShaderID);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(vertexShaderID, maxLength, &maxLength, &infoLog[0]);

		// We don't need the shader anymore.
		glDeleteShader(vertexShaderID);

		printf("%s\n", infoLog.data());
		perror("Vertex shader failed to compile");
	}

	fileContents = "";
	while (std::getline(fragmentFile, line)) {
		fileContents += line + "\n";
	}

	contents = fileContents.c_str();
	glShaderSource(fragmentShaderID, 1, &contents, nullptr);
	glCompileShader(fragmentShaderID);
	isCompiled = 0;
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(fragmentShaderID, maxLength, &maxLength, &infoLog[0]);

		// We don't need the shader anymore.
		glDeleteShader(fragmentShaderID);

		printf("%s\n", infoLog.data());
		perror("Fragment shader failed to compile");
	}

	// Vertex and fragment shaders are successfully compiled.
	// Now time to link them together into a program.
	// Get a program object.
	programID = glCreateProgram();

	// Attach our shaders to our program
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);

	// Link our program
	glLinkProgram(programID);

	// Note the different functions here: glGetProgram* instead of glGetShader*.
	GLint isLinked = 0;
	glGetProgramiv(programID, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(programID, maxLength, &maxLength, &infoLog[0]);

		// We don't need the program anymore.
		glDeleteProgram(programID);
		// Don't leak shaders either.
		glDeleteShader(vertexShaderID);
		glDeleteShader(fragmentShaderID);

		printf("%s\n", infoLog.data());
		perror("Shaders failed to link");
	}

	// Always detach shaders after a successful link.
	glDetachShader(programID, vertexShaderID);
	glDetachShader(programID, fragmentShaderID);
	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);
}

GLint GLSLProgram::getUniformLocation(const std::string& uniformName)
{
	auto location = glGetUniformLocation(programID, uniformName.c_str());
	if (location == GL_INVALID_INDEX) {
		perror(uniformName.c_str());
	}
	return location;
}

void GLSLProgram::use()
{
	glUseProgram(programID);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void GLSLProgram::unuse()
{
	glUseProgram(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

}
