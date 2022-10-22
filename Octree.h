#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <list>

#include "BinaryLoader.h"
#include "Node.h"


class Octree {
	
public:
	Node* root;
	NiftiFile* nf;
	int hdim;
	
	Octree(NiftiFile* nf);
	~Octree();
	float searchPointGetIntensity(glm::vec3 point);
};




