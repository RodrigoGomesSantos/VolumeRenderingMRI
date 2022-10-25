#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <list>

#include "BinaryLoader.h"

class Node {
	
public:
	
	Node** branches;
	float high_value;						//this value is always initialized as 0.0f, only altered on updateNodesValue() call
	float low_value;						//this value is always initialized as 0.0f, only altered on updateNodesValue() call
	glm::vec3 upper;	// upper and lower define the cordinates of the bounds of the octants  
	glm::vec3 lower;	
	int depth;
	Node* parent;

	Node(int max_depth,int hdim, NiftiFile* nf);
	Node(Node* parent, int depth, int max_depth, glm::vec3 lower, glm::vec3 upper);
	void NodeConstructorAuxiliar(int max_depth);
	void updateNodesValue(int hdim, NiftiFile* nf);
	bool isInside(glm::vec3 p);
	bool isLeaf();
	float searchPointGetIntensity(glm::vec3 point);
	~Node();

};