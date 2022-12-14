

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Shader.h"

//Load nifti2 file
#include "BinaryLoader.h"
#include "Node.h"
#include "Octree.h"

// settings
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;
const glm::vec4 BACKGROUND_COLOUR = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

//camera global variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos);

glm::vec3 up = glm::vec3(-1.0f, 0.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, up));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));

//time variables
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

struct Data {
	float viewplane_distance = 2.0f;
	float view_angle = M_PI / 4;
	
	//this points to the top left corner of the screen
	//float real_screen_width = 2 * std::tan(view_angle) * viewplane_distance;
	float real_screen_width = 2 * std::tan(view_angle);
	float real_screen_height = real_screen_width * SCR_HEIGHT / SCR_WIDTH;

	/*glm::vec3 top_left_corner = cameraPos + (viewplane_distance * cameraFront) +
		(real_screen_width / 2) * (-cameraRight)
		+ (cameraUp * (real_screen_height / 2));
	*/	
	glm::vec3 top_left_corner = cameraPos +
		(real_screen_width / 2) * (-cameraRight)
		+ (cameraUp * (real_screen_height / 2));

	int samples_per_ray = 100;
	float sample_distance = viewplane_distance / samples_per_ray; //sample distance
	
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

float nearestNeighbor(NiftiFile* nf, glm::vec3 p, int max);
void volumePrepareForPipeline(float* voxels, float* volume_dimensions, NiftiFile* nf);
glm::vec4 sphereTest(float* volume_dimensions, int x, int y, int z);
glm::vec4 niftiColorTest(NiftiFile* nf, int x, int y, int z);
void prepScreenPixColoursForPipelineOld(NiftiFile* nf, Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data);
void prepScreenPixColoursForPipeline(Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data);
void update_colors(Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data, int layer);


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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Nifti 2 Volume Viewer", NULL, NULL);
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
	/*
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);
	*/

	//matrices
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-0.5f, -0.5f, -0.5f));
	//model = glm::translate(model, glm::vec3(0.0f, 0.0f, 2.0f));

	glm::mat4 view;
	view = glm::lookAt(cameraPos,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 projection;
	//projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10.0f);


	// build and compile our shader program
	// ------------------------------------
	Shader ourShader("3.3.corner_shader.vs", "3.3.corner_shader.fs"); // you can name your shader files however you like


	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------

	/*
	float screen_corners[] =
	{
		//positions			//colors
		1.0f,	1.0f,	0.0f,	0.0f,	0.0f,	1.0f,    // top right
		1.0f,	-1.0f,	0.0f,	0.0f,	0.0f,	1.0f,   // bottom right
		-1.0f,	-1.0f,	0.0f,	0.0f,	1.0f,	0.0f,   // bottom left
		-1.0f,	1.0f,	0.0f,	0.0f,	1.0f,	0.0f    // top left
	};

	unsigned int indices[] = {  // note that we start from 0!
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
	};
	*/

	//store the resulting colors
	//float* all_screen = (float*)malloc(SCR_WIDTH * SCR_HEIGHT * sizeof(float));


	//File Loading 
	NiftiFile nf = NiftiFile("avg152T1_LR_nifti2.nii");
	float* pixels = (float*)malloc(sizeof(float) * SCR_WIDTH * SCR_HEIGHT * 7);

	//float volume_dimensions[3] = { nf.header.dim[1], nf.header.dim[2], nf.header.dim[3] };
	//float volume_dimensions_total_size = volume_dimensions[0] * volume_dimensions[1] * volume_dimensions[2];
	//float* voxels = (float*)malloc(sizeof(float) * volume_dimensions_total_size * 7);
	//volumePrepareForPipeline(voxels, volume_dimensions, &nf);

	////enable this to generate OCTREE

	int time_start = glfwGetTime();
	Octree octree = Octree(&nf); //generates an octree for the nifti file
	int duration = glfwGetTime() - time_start;
	std::cout << "octree creation duration: " << duration << std::endl;



	std::cout << "preparing Pixel Colors fo Pipeline...";
	glm::vec3* screen_pixel_ray_dir = (glm::vec3*)malloc(sizeof(glm::vec3) * SCR_WIDTH * SCR_HEIGHT);
	glm::vec4* screen_pixel_color = (glm::vec4*)malloc(sizeof(glm::vec4) * SCR_WIDTH * SCR_HEIGHT);
	Data d = Data();

	time_start = glfwGetTime();
	prepScreenPixColoursForPipeline(&octree, pixels, screen_pixel_ray_dir, screen_pixel_color, &d);
	duration = glfwGetTime() - time_start;
	std::cout << "pixel creation duration: " << duration << std::endl;
	
	std::cout << "DONE " << std::endl;

	unsigned int VBO, VAO;// EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(screen_corners), screen_corners, GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, 7 * sizeof(float) * volume_dimensions_total_size, voxels, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, 7 * sizeof(float) * SCR_WIDTH * SCR_WIDTH, pixels, GL_DYNAMIC_DRAW);

	/*
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	*/

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// color attribute
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	int layer = 0;

	// render loop
	// -----------
	std::cout << "Rendering" << std::endl;
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
		//clear last frame and zbuffer
		glClearColor(BACKGROUND_COLOUR.r, BACKGROUND_COLOUR.g, BACKGROUND_COLOUR.b, BACKGROUND_COLOUR.a);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);


		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		// send the matrices to the shader (this is usually done each frame since transformation matrices tend to change a lot)
		int modelLoc = glGetUniformLocation(ourShader.ID, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		int viewLoc = glGetUniformLocation(ourShader.ID, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


		//animation
		//model = glm::rotate(model, glm::radians(0.1f), glm::vec3(0.0f, 1.0f, 0.0f));

		//TESTING HERE

		glBufferData

		update_colors(&octree, pixels, screen_pixel_ray_dir, screen_pixel_color, &d, layer);
		layer++;

		//activate the shader
		ourShader.use();

		// render the Volume

		glBindVertexArray(VAO);
		//glDrawArrays(GL_POINTS, 0, volume_dimensions_total_size); //for volume
		glDrawArrays(GL_POINTS, 0, SCR_WIDTH * SCR_HEIGHT); //for screen pixels
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); //to draw from EBO
		glBindVertexArray(0); //no need to unbind everytime

		// glfw: swap buffers look
		//  poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	//glDeleteProgram(shaderProgram);

	free(pixels);//free screen pixels
	free(screen_pixel_color);
	free(screen_pixel_ray_dir);
	//free(voxels);
	//free(all_screen);
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = M_PI * 50.0f * deltaTime;
	glm::mat4 rotationMat = glm::mat4(1.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)

		rotationMat = glm::rotate(rotationMat, glm::radians(cameraSpeed), cameraRight);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		rotationMat = glm::rotate(rotationMat, glm::radians(cameraSpeed), -cameraRight);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		rotationMat = glm::rotate(rotationMat, glm::radians(cameraSpeed), cameraUp);

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		rotationMat = glm::rotate(rotationMat, glm::radians(cameraSpeed), -cameraUp);


	cameraPos = glm::vec4(cameraPos, 1.0f) * rotationMat;

	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos); //keep looking at the center of the world
	cameraRight = glm::normalize(glm::cross(cameraUp, cameraFront));
	cameraUp = glm::cross(cameraFront, cameraRight);

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


void volumePrepareForPipeline(float* voxels, float* volume_dimensions, NiftiFile* nf)
{
	//define the voxel color and position
	for (int x = 0; x < volume_dimensions[0]; x++) {
		for (int y = 0; y < volume_dimensions[1]; y++) {
			for (int z = 0; z < volume_dimensions[2]; z++) {
				int index = (x * volume_dimensions[1] * volume_dimensions[2] + y * volume_dimensions[2] + z) * 7;

				/*x*/voxels[index + 0] = ((float)x) / volume_dimensions[0];
				/*y*/voxels[index + 1] = ((float)y) / volume_dimensions[1];
				/*z*/voxels[index + 2] = ((float)z) / volume_dimensions[2];

				glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				//color = sphereTest(volume_dimensions, x, y, z);
				color = niftiColorTest(nf, x, y, z);

				/*r*/voxels[index + 3] = color.r;
				/*g*/voxels[index + 4] = color.g;
				/*b*/voxels[index + 5] = color.b;
				/*a*/voxels[index + 6] = color.a;
			}
		}
	}
}

glm::vec4 niftiColorTest(NiftiFile* nf, int x, int y, int z) {
	glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	float intensity = nf->volume[nf->transformVector3Position(glm::vec3(x, y, z))] / nf->header.cal_max;

	if (intensity >= 0.1f && intensity < 0.3f)
		color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
	if (intensity >= 0.3f && intensity < 0.4f)
		color = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f);
	if (intensity >= 0.4f && intensity < 0.5f)
		color = glm::vec4(0.8f, 0.8f, 0.4f, 1.0f);
	if (intensity >= 0.5f && intensity < 0.6f)
		color = glm::vec4(0.1f, 0.5f, 0.5f, 1.0f);
	if (intensity >= 0.6f && intensity < 0.7f)
		color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	if (intensity >= 0.7f && intensity <= 1.0f)
		color = glm::vec4(0.9f, 0.5f, 0.5f, 1.0f);
	return color;
}


glm::vec4 sphereTest(float* volume_dimensions, int x, int y, int z) {
	glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 center = glm::vec3(volume_dimensions[0] / 2, volume_dimensions[1] / 2, volume_dimensions[2] / 2);
	float radius = volume_dimensions[0] / 2;
	float sphere_test = pow(x - center.x, 2) + pow(y - center.y, 2) + pow(z - center.z, 2);
	if (sphere_test <= pow(radius, 2)) {

		if (x > volume_dimensions[0] / 2)
			if (y > volume_dimensions[1] / 2)
				if (z > volume_dimensions[2] / 2)
					color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				else
					color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
			else
				if (z > volume_dimensions[2] / 2)
					color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
				else
					color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		else
			if (y > volume_dimensions[1] / 2)
				if (z > volume_dimensions[2] / 2)
					color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
				else
					color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
			else
				if (z > volume_dimensions[2] / 2)
					color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				else
					color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	if (z == 0 || z == volume_dimensions[2] - 1)
		color = glm::vec4(1.0f - BACKGROUND_COLOUR.r, 1.0f - BACKGROUND_COLOUR.g, 1.0f - BACKGROUND_COLOUR.b, 1.0f);

	return color;
}


//returns a float normalized between 0.0f and 1.0f representing intensity
//TODO - fix this for ray direction and not dataset direction
float nearestNeighbor(NiftiFile* nf, glm::vec3 p, int max) {

	p.x = (int)p.x * nf->header.dim[1] / SCR_WIDTH;
	p.y = (int)p.y * nf->header.dim[2] / SCR_HEIGHT;
	p.z = (int)p.z * nf->header.dim[3] / max;

	return  (nf->volume[nf->transformVector3Position(p)]) / nf->header.cal_max;
}


void prepScreenPixColoursForPipelineOld(NiftiFile* nf, Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data) {


	//matrices
	glm::mat4 modelAux = glm::mat4(1.0f);
	modelAux = glm::translate(modelAux, glm::vec3(0.5f, 0.5f, 0.5f));
	//model = glm::translate(model, glm::vec3(0.0f, 0.0f, 2.0f));

	glm::mat4 viewAux;
	viewAux = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 0.0f),
		cameraPos,
		glm::vec3(0.0f, 1.0f, 0.0f));



	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			screen_pixel_ray_dir[x * SCR_HEIGHT + y] = normalize(data->top_left_corner +
				(x * data->real_screen_width / SCR_WIDTH) * (cameraRight)+
				(y * data->real_screen_height / SCR_HEIGHT) * (-up)
				- cameraPos);
		}
	}

	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
	float radius = 0.5f;



	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			//glm::vec4 fragmentColor = BACKGROUND_COLOUR;
			glm::vec4 fragmentColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);

			//float highest_intensity = 0.0f;
			for (int i = 0; i < data->samples_per_ray; i++) {
				glm::vec3 auxPos = cameraPos + i * data->sample_distance * screen_pixel_ray_dir[x * SCR_HEIGHT + y]; //this is the position of the sample in world cordinates
				/*
				float intensity = octree->searchPointGetIntensity(modelAux * viewAux * glm::vec4(auxPos, 1.0f));
				if (intensity > highest_intensity)
					highest_intensity = intensity;*/

				float sphere_test = pow(auxPos.x - center.x, 2) + pow(auxPos.y - center.y, 2) + pow(auxPos.z - center.z, 2);
				if (sphere_test <= pow(radius, 2)) {
					fragmentColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				}
			}

			//if (highest_intensity > 250.0f)


			screen_pixel_color[x * SCR_HEIGHT + y] = fragmentColor;
		}
	}

	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			int index = (x * SCR_HEIGHT + y) * 7;
			/*x*/pixels[index + 0] = 2 * (((float)x) / SCR_WIDTH) - 1;
			/*y*/pixels[index + 1] = 2 * (((float)y) / SCR_HEIGHT) - 1;
			/*z*/pixels[index + 2] = 0.0f;
			/*r*/pixels[index + 3] = screen_pixel_color[x * SCR_HEIGHT + y].r;
			/*g*/pixels[index + 4] = screen_pixel_color[x * SCR_HEIGHT + y].g;
			/*b*/pixels[index + 5] = screen_pixel_color[x * SCR_HEIGHT + y].b;
			/*a*/pixels[index + 6] = screen_pixel_color[x * SCR_HEIGHT + y].a;
		}
	}


}

void prepScreenPixColoursForPipeline(Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data) {

	//matrices
	glm::mat4 modelAux = glm::mat4(1.0f);
	modelAux = glm::translate(modelAux, glm::vec3(0.5f, 0.5f, 0.5f));

	glm::mat4 viewAux;
	viewAux = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 0.0f),
		cameraPos,
		cameraUp);

	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			screen_pixel_ray_dir[x * SCR_HEIGHT + y] = cameraFront;
				/*
				normalize(data->top_left_corner +
				(x * data->real_screen_width / SCR_WIDTH) * (cameraRight) +
				(y * data->real_screen_height / SCR_HEIGHT) * (-cameraUp)
				- cameraPos);*/
		}
	}



	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			glm::vec4 fragmentColor = BACKGROUND_COLOUR;

			/*
			for (int i = 0; i < data->samples_per_ray; i++) {
				//glm::vec3 auxPos = cameraPos + i * data->sample_distance * screen_pixel_ray_dir[x * SCR_HEIGHT + y]; //this is the position of the sample in world cordinates
				glm::vec3 auxPos = data->top_left_corner + 
					x * data->real_screen_width / SCR_WIDTH * cameraRight + 
					y * data->real_screen_height /SCR_HEIGHT * (- cameraUp) + 
					i * data->sample_distance * screen_pixel_ray_dir[x * SCR_HEIGHT + y]; //this is the position of the sample in world cordinates
				float intensity = octree->searchPointGetIntensity(modelAux * glm::vec4(auxPos, 1.0f));

				//blend colours
				glm::vec4 blend_color = glm::vec4(0.0f);

				if (intensity > 25.0f)
					blend_color = glm::vec4(0.0f, 0.5f, 1.0f, 0.5f);
				if (intensity > 50.0f)
					blend_color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
				if (intensity > 100.0f)
					blend_color = glm::vec4(0.0f, 1.0f, 0.0f, 0.5f);
				if (intensity > 150.0f)
					blend_color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
				if (intensity > 200.0f)
					blend_color = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
				if (intensity > 250.0f)
					blend_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

				fragmentColor = glm::vec4(
					fragmentColor.r * (1 - blend_color.a) + blend_color.r * blend_color.a,
					fragmentColor.g * (1 - blend_color.a) + blend_color.g * blend_color.a,
					fragmentColor.b * (1 - blend_color.a) + blend_color.b * blend_color.a,
					1.0f
				);

			}*/
			glm::vec3 auxPos = data->top_left_corner +
				x * data->real_screen_width / SCR_WIDTH * cameraRight +
				y * data->real_screen_height / SCR_HEIGHT * (-cameraUp) +
				data->samples_per_ray/2 * data->sample_distance * screen_pixel_ray_dir[x * SCR_HEIGHT + y]; //this is the position of the sample in world cordinates
			float intensity = octree->searchPointGetIntensity(modelAux * glm::vec4(auxPos, 1.0f));

			//blend colours
			glm::vec4 blend_color = glm::vec4(0.0f);

			if (intensity > 25.0f)
				blend_color = glm::vec4(0.0f, 0.5f, 1.0f, 0.5f);
			if (intensity > 50.0f)
				blend_color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
			if (intensity > 100.0f)
				blend_color = glm::vec4(0.0f, 1.0f, 0.0f, 0.5f);
			if (intensity > 150.0f)
				blend_color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
			if (intensity > 200.0f)
				blend_color = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
			if (intensity > 250.0f)
				blend_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

			fragmentColor = glm::vec4(
				fragmentColor.r * (1 - blend_color.a) + blend_color.r * blend_color.a,
				fragmentColor.g * (1 - blend_color.a) + blend_color.g * blend_color.a,
				fragmentColor.b * (1 - blend_color.a) + blend_color.b * blend_color.a,
				1.0f
			);

			screen_pixel_color[x * SCR_HEIGHT + y] = fragmentColor;

		}
	}

	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			int index = (x * SCR_HEIGHT + y) * 7;
			/*x*/pixels[index + 0] = 2 * (((float)x) / SCR_WIDTH) - 1;
			/*y*/pixels[index + 1] = 2 * (((float)y) / SCR_HEIGHT) - 1;
			/*z*/pixels[index + 2] = 0.0f;
			/*r*/pixels[index + 3] = screen_pixel_color[x * SCR_HEIGHT + y].r;
			/*g*/pixels[index + 4] = screen_pixel_color[x * SCR_HEIGHT + y].g;
			/*b*/pixels[index + 5] = screen_pixel_color[x * SCR_HEIGHT + y].b;
			/*a*/pixels[index + 6] = screen_pixel_color[x * SCR_HEIGHT + y].a;
		}
	}


}

void update_colors(Octree* octree, float* pixels, glm::vec3* screen_pixel_ray_dir, glm::vec4* screen_pixel_color, Data* data, int layer) {

	//matrices
	glm::mat4 modelAux = glm::mat4(1.0f);
	modelAux = glm::translate(modelAux, glm::vec3(0.5f, 0.5f, 0.5f));

	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			glm::vec4 fragmentColor = BACKGROUND_COLOUR;

			glm::vec3 auxPos = data->top_left_corner +
				x * data->real_screen_width / SCR_WIDTH * cameraRight +
				y * data->real_screen_height / SCR_HEIGHT * (-cameraUp) +
				(layer % data->samples_per_ray) * data->sample_distance * screen_pixel_ray_dir[x * SCR_HEIGHT + y]; //this is the position of the sample in world cordinates
			float intensity = octree->searchPointGetIntensity(modelAux * glm::vec4(auxPos, 1.0f));

			//blend colours
			glm::vec4 blend_color = glm::vec4(0.0f);

			if (intensity > 25.0f)
				blend_color = glm::vec4(0.0f, 0.5f, 1.0f, 0.5f);
			if (intensity > 50.0f)
				blend_color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
			if (intensity > 100.0f)
				blend_color = glm::vec4(0.0f, 1.0f, 0.0f, 0.5f);
			if (intensity > 150.0f)
				blend_color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
			if (intensity > 200.0f)
				blend_color = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
			if (intensity > 250.0f)
				blend_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

			fragmentColor = glm::vec4(
				fragmentColor.r * (1 - blend_color.a) + blend_color.r * blend_color.a,
				fragmentColor.g * (1 - blend_color.a) + blend_color.g * blend_color.a,
				fragmentColor.b * (1 - blend_color.a) + blend_color.b * blend_color.a,
				1.0f
			);

			screen_pixel_color[x * SCR_HEIGHT + y] = fragmentColor;

		}
	}

	for (int x = 0; x < SCR_WIDTH; x++) {
		for (int y = 0; y < SCR_HEIGHT; y++) {
			int index = (x * SCR_HEIGHT + y) * 7;
			/*x*/pixels[index + 0] = 2 * (((float)x) / SCR_WIDTH) - 1;
			/*y*/pixels[index + 1] = 2 * (((float)y) / SCR_HEIGHT) - 1;
			/*z*/pixels[index + 2] = 0.0f;
			/*r*/pixels[index + 3] = screen_pixel_color[x * SCR_HEIGHT + y].r;
			/*g*/pixels[index + 4] = screen_pixel_color[x * SCR_HEIGHT + y].g;
			/*b*/pixels[index + 5] = screen_pixel_color[x * SCR_HEIGHT + y].b;
			/*a*/pixels[index + 6] = screen_pixel_color[x * SCR_HEIGHT + y].a;
		}
	}



}