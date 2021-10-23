// #include <SOIL/SOIL.h>

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

// // Create a texture from an image file
// GLuint loadTexture(const GLchar* path);
// GLuint loadTexture(const GLchar* path)
// {
// 	GLuint texture;
// 	glGenTextures(1, &texture);

// 	int width, height;
// 	unsigned char* image;

// 	sf::Image image;

// 	glBindTexture(GL_TEXTURE_2D, texture);
// 	image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGB);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
// 	SOIL_free_image_data(image);

// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// 	return texture;
// }

GLuint createShader(GLenum type, const GLchar* src);
GLuint createShader(GLenum type, const GLchar* src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	return shader;
}

void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& shaderProgram);
void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& shaderProgram)
{
	// Create and compile the vertex shader
	vertexShader = createShader(GL_VERTEX_SHADER, vertSrc);

	// Create and compile the fragment shader
	fragmentShader = createShader(GL_FRAGMENT_SHADER, fragSrc);

	// Link the vertex and fragment shader into a shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);

	char buffer[512];
	glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
	glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
}

void createShaderProgramWithGeom(const GLchar* vertSrc, const GLchar* fragSrc, const GLchar* geomSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& geometryShader, GLuint& shaderProgram);
void createShaderProgramWithGeom(const GLchar* vertSrc, const GLchar* fragSrc, const GLchar* geomSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& geometryShader, GLuint& shaderProgram)
{
	// Compile shaders
	vertexShader = createShader(GL_VERTEX_SHADER, vertSrc);
	fragmentShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
	geometryShader = createShader(GL_GEOMETRY_SHADER, geomSrc);

	// Link the vertex and fragment shader into a shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, geometryShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);

	char buffer[512];
	glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
	glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
}

void specifySceneVertexAttributes(GLuint shaderProgram);
void specifySceneVertexAttributes(GLuint shaderProgram)
{
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint brightAttrib = glGetAttribLocation(shaderProgram, "brightness");
	glEnableVertexAttribArray(brightAttrib);
	glVertexAttribPointer(brightAttrib, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
}

void specifyScreenVertexAttributes(GLuint shaderProgram);
void specifyScreenVertexAttributes(GLuint shaderProgram)
{
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
}
