

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>

//Load nifti2 file
#include "BinaryLoader.h"
#include "Node.h"
#include "Octree.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

float nearestNeighbor(NiftiFile* nf, glm::vec3 p, int max);
void volumePrepareForPipeline(float* pixels, float* voxels, NiftiFile* nf);

// settings
const unsigned int SCR_WIDTH = 900;
const unsigned int SCR_HEIGHT = 900;
const glm::vec3 BACKGROUND_COLOUR = glm::vec3(0.2f, 0.3f, 0.3f);

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec4 aColor;\n"
"\n"
"out vec4 ourColor;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec4 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = ourColor;\n"
"}\n\0";

//program global variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);

glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraFront));
glm::vec3 cameraUp = glm::cross(cameraFront, cameraRight);

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame


int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);

	//matrices
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-0.5f, -0.5f, -0.5f));

	glm::mat4 view;
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 projection;
	projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);



	// build and compile our shader program
	// ------------------------------------
	// vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// link shaders
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------


	/*
	float screen_corners[] =
		{
		// positions         // colors
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
		-1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
		 1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f    // top right
		-1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f    // top left
	}
	*/

	//File Loading 
	NiftiFile nf = NiftiFile("avg152T1_LR_nifti2.nii");
	float* pixels = (float*)malloc(sizeof(float) * SCR_WIDTH * SCR_HEIGHT * 6); //currently not used for anything
	float* voxels = (float*)malloc(sizeof(float) * nf.header.dim[1] * nf.header.dim[2] * nf.header.dim[3] * 7);


	volumePrepareForPipeline(pixels, voxels, &nf);

	unsigned int VBO, VAO;// EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float) * SCR_WIDTH * SCR_WIDTH, pixels, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, 7 * sizeof(float) * nf.header.dim[1] * nf.header.dim[2] * nf.header.dim[3], voxels, GL_STATIC_DRAW);

	/*
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	*/

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	//HERE
	int time_start = glfwGetTime();

	Octree octree(&nf); //generates an octree for the nifti file

	int duration = glfwGetTime() - time_start;
	std::cout << "octree creation duration: " << duration << std::endl;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		//update delta time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(BACKGROUND_COLOUR.r, BACKGROUND_COLOUR.g, BACKGROUND_COLOUR.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		// send the matrices to the shader (this is usually done each frame since transformation matrices tend to change a lot)
		int modelLoc = glGetUniformLocation(shaderProgram, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		int viewLoc = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		//animation
		//model = glm::rotate(model, glm::radians(0.1f), glm::vec3(0.0f, 1.0f, 0.0f));

		//activate the shader
		glUseProgram(shaderProgram);

		// render the Volume
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, SCR_WIDTH * SCR_HEIGHT);
		//glBindVertexArray(0); //no need to unbind everytime


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}



	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);

	free(pixels);//free screen pixels
	free(voxels);
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	float cameraSpeed = 2.5f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos);
	cameraRight = glm::normalize(glm::cross(cameraUp, cameraFront));
	cameraUp = glm::cross(cameraFront, cameraRight);


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

}




// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


/**
* 
*/
void volumePrepareForPipeline(float* pixels, float* voxels, NiftiFile* nf)
{
	//define the voxel color and position
	for (int i = 0; i < nf->header.dim[1]; i++) {
		for (int j = 0; j < nf->header.dim[2]; j++) {
			for (int k = 0; k < nf->header.dim[3]; k++) {
				int aux = (i * nf->header.dim[2] * nf->header.dim[3] + j * nf->header.dim[3] + k) * 7;

				/*x*/voxels[aux + 0] = ((float)i) / nf->header.dim[1];
				/*y*/voxels[aux + 1] = ((float)j) / nf->header.dim[2];
				/*z*/voxels[aux + 2] = ((float)k) / nf->header.dim[3];

				float intensity = nf->volume[nf->transformVector3Position(glm::vec3(i, j, k))] / nf->header.cal_max;

				glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

				if (intensity >= 0.09f)
					color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
				if (intensity >= 0.3 && intensity < 0.4)
					color = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f);
				if (intensity >= 0.4 && intensity < 0.5)
					color = glm::vec4(0.8f, 0.8f, 0.4f, 1.0f);
				if (intensity >= 0.5 && intensity < 0.6)
					color = glm::vec4(0.1f, 0.5f, 0.5f, 1.0f);
				if (intensity >= 0.6 && intensity < 0.7)
					color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				if (intensity >= 0.7 && intensity < 0.1)
					color = glm::vec4(0.9f, 0.5f, 0.5f, 1.0f);

				//atributes the average value of the (x,y) cordinate
				/*r*/voxels[aux + 3] = color[0];//sum / max_depth; //nf.volume[nf.transformVector3Position(glm::vec3(i,100,j))]/255.0f;
				/*g*/voxels[aux + 4] = color[1];
				/*b*/voxels[aux + 5] = color[2];
				/*alpha*/voxels[aux + 6] = color[3];
			}
		}
	}
}

//returns a float normalized between 0.0f and 1.0f representing intensity
//TODO - fix this for ray direction and not dataset direction
float nearestNeighbor(NiftiFile* nf, glm::vec3 p, int max) {

	p.x = (int)p.x * nf->header.dim[1] / SCR_WIDTH;
	p.y = (int)p.y * nf->header.dim[2] / SCR_HEIGHT;
	p.z = (int)p.z * nf->header.dim[3] / max;

	return  (nf->volume[nf->transformVector3Position(p)]) / nf->header.cal_max;
}





