#include "Octree.h"

Octree::Octree(NiftiFile* nf) {

	this->nf = nf;

	//get the highest dimension
	this->hdim = 0;
	for (int i = 0; i < 3; i++) {
		if (hdim < nf->header.dim[i + 1])
			hdim = nf->header.dim[i + 1];
	}

	int max_depth = 0;	
	while (std::pow(2, max_depth) < hdim)
		max_depth++;

	this->root = (Node*) malloc(sizeof(Node));
	this->root = new Node(max_depth,hdim,nf);
}

float Octree::searchPointGetIntensity(glm::vec3 point) {
	
	/*
	if (root->isInside(point)) {
	}
		for (int i = 0; i < 8; i++) {
			root->branches[0][i]->
		}		
	*/

	return 0.0f; //TODO
}

Octree::~Octree() {
	free(root);
}


