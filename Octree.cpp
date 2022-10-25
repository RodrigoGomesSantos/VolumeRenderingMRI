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
	this->root = new Node(max_depth,pow(2,max_depth), nf); //HERE changed hdim to 128
	std::cout << "Octree Root High|Low Values: " << this->root->high_value << "|" << this->root->low_value << std::endl;
}


float Octree::searchPointGetIntensity(glm::vec3 point) {
	float res = 0.0f;
	res = root->searchPointGetIntensity(point);
	return res;
}

Octree::~Octree() {
	free(root);
}


