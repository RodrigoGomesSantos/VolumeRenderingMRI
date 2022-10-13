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

	std::cout<< "MAX_DEPTH: " << max_depth<< std::endl;

	this->root = (Node*) malloc(sizeof(Node));
	this->root[0] = Node(max_depth,hdim,nf);
}

Octree::~Octree() {
	free(root);
}


