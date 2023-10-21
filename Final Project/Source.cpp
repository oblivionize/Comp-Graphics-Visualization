#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

#include <glm/gtc/constants.hpp>
// STL
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Camera header
#include "camera.h" ;


using namespace std; // Uses the standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

GLuint shaderProgram, lampProgram;

//Texture
GLuint textureId1, textureId2, textureId3, textureId4,textureId5, textureId6,textureId7;

GLint gTexWrapMode = GL_REPEAT;
GLuint VBO, VAO, VAO2, VBO2, PlaneVertices, keyboardVertices,VAO3, VBO3,pyramidVerticies;
const char* const WINDOW_TITLE = "Module 5 Milestone"; // Macro for window title

// Variables for window width and height
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

//Camera variables
Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;
bool orthographic = false;

//Timing variables
float gDeltaTime = 0.0f;
float gLastFrame = 0.0f;

// Subject position and scale
glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
glm::vec3 gCubeScale(2.0f);

// Cube and light color
//m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

// Light position and scale
glm::vec3 gLightPosition(2.5f, 0.5f, -0.0f);
glm::vec3 gLightScale(0.3f);

//function declarations
bool UInitialize(int, char* [], GLFWwindow** window);
void UCreateShaderProgram(const char* vertex, const char* fragment, GLuint& program);
void UCreateMesh();
void UCreatePlaneMesh();
void UCreatePyramidMesh();
void URender(void);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
bool UCreateTexture(const char* filename, GLuint& textureId);

/* Cube Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting*/
	float ambientStrength = 0.1f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse = impact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensity = 0.8f; // Set specular light strength
	float highlightSize = 16.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	// Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	// Calculate phong result
	vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


int main(int argc, char* argv[]) {

	//Intialize the window
	GLFWwindow* window = nullptr;
	if (!UInitialize(argc, argv, &window))
		return -1;

	//Create mesh
	UCreateMesh();
	UCreatePlaneMesh();
	UCreatePyramidMesh();


	// Create the shader program
	UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, shaderProgram);

	// Create the lamp shader program
	UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, lampProgram);

	// Load textures
	const char* texFilename = "keyboard.png";
	if (!UCreateTexture(texFilename, textureId1))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	// Load textures
	texFilename = "desktop.jpg";
	if (!UCreateTexture(texFilename, textureId2))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	// Load textures
	texFilename = "cable.jpg";
	if (!UCreateTexture(texFilename, textureId3))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	texFilename = "monitor.png";
	if (!UCreateTexture(texFilename, textureId4))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	texFilename = "cup.jpg";
	if (!UCreateTexture(texFilename, textureId5))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	texFilename = "computerfront.jpg";
	if (!UCreateTexture(texFilename, textureId6))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	texFilename = "brick.png";
	if (!UCreateTexture(texFilename, textureId7))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

	//set background to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);



	//render loop
	while (!glfwWindowShouldClose(window)) {



		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		//Input
		UProcessInput(window);

		URender();







		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);    // Flips the the back buffer with the front buffer every frame.
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);

}

bool UCreateTexture(const char* filename, GLuint& textureId) {
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{


		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}

bool UInitialize(int argc, char* argv[], GLFWwindow** window) {

	//GLFW initialization
	glfwInit();

	//GLFW window creation
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	//error handling
	if (*window == NULL) {
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return false;
	}
	int screenWidth, screenHeight;


	glfwMakeContextCurrent(*window);
	glfwGetFramebufferSize(*window, &screenWidth, &screenHeight);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	//Initialize GLEW
	glewExperimental = true;

	//In case GLEW fails to intialize
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		glfwTerminate();
		return false;
	}

	//return true if initialize is successful
	return true;

}

void UCreateShaderProgram(const char* vertex, const char* fragment, GLuint& program) {

	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	program = glCreateProgram();

	//create shader objects
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	//shader source
	glShaderSource(fragmentShader, 1, &fragment, NULL);
	glShaderSource(vertexShader, 1, &vertex, NULL);

	//compile shaders
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit;

	}
	glCompileShader(fragmentShader);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit;

	}


	//program for shader	
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;


	}

	glUseProgram(program);
}

void UCreateMesh() {

	// Vertex data
	GLfloat verts[] = {
		//positions           //normals			  Texture Coords.
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,


	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,


	  -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,


	  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,


	 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
	  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
	  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,


	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f

	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	keyboardVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//active 2 buffers
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);


}

void UCreatePlaneMesh() {

	// Position and Color data
	GLfloat verts[] = {
		// Vertex Positions //normals				/textures
		5.0f, 5.0f, 0.0f,	0.0f,  0.0f, -1.0f,		1.0f,1.0f,
		5.0f,-5.0f,0.0f,	0.0f,  0.0f, -1.0f,		1.0f,0.0f,
		-5.0f,5.0f,0.0f,	0.0f,  0.0f, -1.0f,		0.0f,1.0f,

		-5.0f, -5.0f, 0.0f,	0.0f,  0.0f, -1.0f,		0.0f,0.0f,
		5.0f,-5.0f,0.0f,	0.0f,  0.0f, -1.0f,		1.0f,0.0f,
		-5.0f,5.0f,0.0f,	0.0f,  0.0f, -1.0f,		0.0f,1.0f,

	};




	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;
	PlaneVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &VAO2);
	glBindVertexArray(VAO2);

	//active 2 buffers
	glGenBuffers(1, &VBO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);




	// Strides between vertex cooordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);


}

void UCreatePyramidMesh() {
	//Posiiton and color data
	GLfloat verts[] = {
		//vertex Positions	  //Normals				//Texture data
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,	  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
		0.0f, 0.5f, 0.0f,	  0.0f,  0.0f, -1.0f,	0.5f, 1.0f,

		-0.5f, -0.5f, 0.5f,	  0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
		0.5f, -0.5f, 0.5f,	  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
		0.0f, 0.5f, 0.0f,	  0.0f,  0.0f,  1.0f,	0.5f, 1.0f,

		-0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,	  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		0.0f, 0.5f, 0.0f,	  -1.0f,  0.0f,  0.0f,	0.5f, 1.0f,

		0.5f, -0.5f, -0.5f,	  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		0.5f, -0.5f, 0.5f,	  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		0.0f, 0.5f, 0.0f,	  1.0f,  0.0f,  0.0f,	0.5f, 1.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,	0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,	  0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
		0.5f, -0.5f, 0.5f,	  0.0f, -1.0f,  0.0f,	0.5f, 1.0f,

		-0.5f, -0.5f, 0.5f,	  0.0f,  1.0f,  0.0f,	0.0f, 0.0f,
		0.5f, -0.5f, 0.5f,	  0.0f,  1.0f,  0.0f,	1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,	0.5f, 1.0f


	};



	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	pyramidVerticies = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &VAO3);
	glBindVertexArray(VAO3);

	//vbo
	glGenBuffers(1, &VBO3);
	glBindBuffer(GL_ARRAY_BUFFER, VBO3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);



	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void URender(void) {
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//tiling 
	glm::vec2 gUVScale(1.0f, 1.0f);
	glm::vec2 gUVScalePlane(4.0f, 4.0f);

	glBindVertexArray(VAO);

	// Set the shader to be used
	glUseProgram(shaderProgram);

	//Keyboard Transformations
	glm::mat4 scale = glm::scale(glm::vec3(3.0f, 0.1f, 1.0f));
	glm::mat4 rotation = glm::rotate(glm::radians(180.0f), glm::vec3(1.0, 0.0f, 0.0f));
	glm::mat4 translation = glm::translate(glm::vec3(0.0f, -1.94f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 model = translation * rotation * scale;

	glm::mat4 view = gCamera.GetViewMatrix();

	glm::mat4 projection;

	// Creates a perspective projection
	projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);



	if (orthographic) {
		float scale = 100;
		projection = glm::ortho(-(800.0f / scale), 800.0f / scale, 600.0f / scale, -(600.0f / scale), -4.0f, 6.5f);
	}
	else {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// Retrieves and passes transform matrices to the Shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	GLint UVScaleLoc = glGetUniformLocation(shaderProgram, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


	// Draws Keyboard
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId1);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Monitor Transformations
	scale = glm::scale(glm::vec3(4.0f, 0.1f, 2.0f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(0.5f, -0.5f, -3.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Monitor
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId4);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Monitor Transformations
	scale = glm::scale(glm::vec3(4.0f, 0.1f, 2.0f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	glm::mat4 rotation2 = glm::rotate(glm::radians(20.0f), glm::vec3(0.0, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(-3.4f, -0.5f, -2.8f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * rotation2 * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Monitor
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle


	// LAMP: draw lamp
	//----------------
	glUseProgram(lampProgram);

	//Transform the smaller cube used as a visual que for the light source
	model = glm::translate(gLightPosition) * glm::scale(gLightScale);

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(lampProgram, "model");
	viewLoc = glGetUniformLocation(lampProgram, "view");
	projLoc = glGetUniformLocation(lampProgram, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices);


	//Draws Cylinder

	//model transformations
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	
	translation = glm::translate(glm::vec3(0.0f, -1.94f, -4.3f));
	model = translation * scale;

	// Set the shader to be used
	glUseProgram(shaderProgram);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId3);



	GLUquadricObj* obj = gluNewQuadric();

	gluQuadricTexture(obj, GL_TRUE);
	glRotatef(-90.0f, 1.0f, 1.0f, 0.0f);
	gluCylinder(obj, 0.03f, 0.03f, 3.0f, 30, 30);

	//Draws Cylinder for cup

	//model transformations
	scale = glm::scale(glm::vec3(0.8f, 0.8f, 0.8f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(-3.0f, -1.94f, -1.0f));
	model = translation * rotation * scale;

	// Set the shader to be used
	glUseProgram(shaderProgram);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId5);



	obj = gluNewQuadric();

	gluQuadricTexture(obj, GL_TRUE);
	gluCylinder(obj, 0.2f, 0.3f, 1.0f, 30, 30);


	//Computer Transformations
	scale = glm::scale(glm::vec3(1.5f, 3.0f, 2.5f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(4.0f, -0.7f, -2.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Computer
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Computer edges Transformations
	scale = glm::scale(glm::vec3(.05f, 2.5f, 0.2f));
	rotation = glm::rotate(glm::radians(-0.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(3.27f, -0.7f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Computer edge
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Computer edges Transformations
	scale = glm::scale(glm::vec3(.05f, 2.5f, 0.2f));
	rotation = glm::rotate(glm::radians(-0.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(4.73f, -0.7f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Computer edges
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Computer edges Transformations
	scale = glm::scale(glm::vec3(.05f, 1.5f, 0.2f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(4.0f, -1.93f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Computer edges
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	// Draws Computer edges
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle

	//Computer edges Transformations
	scale = glm::scale(glm::vec3(.05f, 1.5f, 0.2f));
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(4.0f, 0.524f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Draws Computer edges
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId6);
	glDrawArrays(GL_TRIANGLES, 0, keyboardVertices); // Draws the triangle


	//Draw Pyramid
	glBindVertexArray(VAO3);

	scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	//rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(2.0f, -1.7f, -2.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindTexture(GL_TEXTURE_2D, textureId7);
	glDrawArrays(GL_TRIANGLES, 0, pyramidVerticies);

	//Draw Pyramid
	glBindVertexArray(VAO3);

	scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	rotation = glm::rotate(glm::radians(120.0f), glm::vec3(1.0, 0.0f, 0.0f));
	rotation2 = glm::rotate(glm::radians(10.0f), glm::vec3(0.0, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(1.5f, -1.9f, -2.9f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation *rotation2 * scale;


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindTexture(GL_TEXTURE_2D, textureId7);
	glDrawArrays(GL_TRIANGLES, 0, pyramidVerticies);

	//model transformations
	
	scale = glm::scale(glm::vec3(1.0f, 0.5f, 1.0f));
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));

	translation = glm::translate(glm::vec3(0.0f, -2.0f, -2.0f));

	model = translation * rotation * scale;

	// Set the shader to be used
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO2);
	

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);


	UVScaleLoc = glGetUniformLocation(shaderProgram, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScalePlane));

	//Draw Plane

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId2);


	glDrawArrays(GL_TRIANGLES, 0, PlaneVertices);
	glBindVertexArray(0);

	
	

	//Deactivate the Vertex Array Object and shader program
	glBindVertexArray(0);
	glUseProgram(0);


}

//Process Input
void UProcessInput(GLFWwindow* window)
{

	static const float cameraSpeed = 2.5f;
	bool keypress = false;

	//Close Program when escape is entered.
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//w key (Foward)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	}
	//s key (backwards)
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);

	//a key (Left)
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);

	//d key (Right)
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

	//q key (up)
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);

	//e key (down)
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);

	//p key (ortho)
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		orthographic = !orthographic;
}

//glfw: callback for mouse movement
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos;

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

//glfw: callback for scroll wheel
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	gCamera.ProcessMouseScroll(yoffset);
}
