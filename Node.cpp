#include "Node.h"

//root constructor
Node::Node(int max_depth, int hdim, NiftiFile* nf) {

	this->depth = 0;
	this->high_value = 0.0f;
	this->low_value = 0.0f;
	this->branches = nullptr;
	this->lower = glm::vec3(0.0f, 0.0f, 0.0f);
	this->upper = glm::vec3(1.0f, 1.0f, 1.0f);
	this->parent = nullptr;

	NodeConstructorAuxiliar(max_depth);
	std::cout << " Updating Octree Nodes Value" << std::endl;
	updateNodesValue(hdim, nf);
	std::cout << "Octree Nodes Value Updated" << std::endl;
}

//child node constructor
Node::Node(Node* parent, int depth, int max_depth, glm::vec3 lower, glm::vec3 upper) {
	this->parent = parent;
	this->depth = depth;
	this->high_value = 0.0f;
	this->low_value = 0.0f;
	this->branches = nullptr;
	this->lower = lower;
	this->upper = upper;
	NodeConstructorAuxiliar(max_depth);
}

void Node::NodeConstructorAuxiliar(int max_depth) {

	if (this->depth < max_depth) {
		glm::vec3 distance = upper - lower;
		this->branches = (Node**)malloc(8 * sizeof(Node*));

		if (this->branches != nullptr)
			for (int x = 0; x < 2; x++)
				for (int y = 0; y < 2; y++)
					for (int z = 0; z < 2; z++) {

						glm::vec4 l(lower, 1.0f);
						glm::mat4 trans = glm::mat4(1.0f);
						trans = glm::translate(trans, glm::vec3(x * distance.x / 2, y * distance.y / 2, z * distance.y / 2));
						glm::vec3 child_lower = trans * l;

						glm::vec4 u(child_lower, 1.0f);
						trans = glm::mat4(1.0f);
						trans = glm::translate(trans, glm::vec3(distance.x / 2, distance.y / 2, distance.y / 2));
						glm::vec3 child_upper = trans * u;

						this->branches[x * 4 + y * 2 + z] = new Node(this, this->depth + 1, max_depth, child_lower, child_upper);
					}
		else {
			std::cout << "Unable to allocate memory for some reason" << std::endl;
		}
	}
}


/*
* Auxiliary function to std::max
*/
bool comp(float a, float b)
{
	return (a < b);
}

void Node::updateNodesValue(int hdim, NiftiFile* nf) {

	if (!isLeaf()) {

		for (int i = 0; i < 8; i++) {
			branches[i][0].updateNodesValue(hdim, nf);
		}

		for (int i = 0; i < 8; i++) {
			if (high_value < branches[i][0].high_value) {
				high_value = branches[i][0].high_value;
			}
			if (low_value > branches[i][0].low_value) {
				low_value = branches[i][0].low_value;
			}
		}
	}

	else {// Is Leaf //carefull this value is not true to the data set
		glm::mat4 scaleMatrix = glm::mat4(1.0);
		scaleMatrix = glm::scale(scaleMatrix, glm::vec3(hdim, hdim, hdim));
		glm::vec3 res = scaleMatrix * glm::vec4(lower, 1);

		//center the dataset on the volume
		if (
			res.x >= (hdim / 2.0f) - (nf->header.dim[1] / 2.0f) && res.x < (hdim / 2.0f) + (nf->header.dim[1] / 2.0f) &&
			res.y >= (hdim / 2.0f) - (nf->header.dim[2] / 2.0f) && res.y < (hdim / 2.0f) + (nf->header.dim[2] / 2.0f) &&
			res.z >= (hdim / 2.0f) - (nf->header.dim[3] / 2.0f) && res.z < (hdim / 2.0f) + (nf->header.dim[3] / 2.0f))
		{

			res = glm::vec3(
				(int) (res.x + (nf->header.dim[1] / 2.0f) - hdim / 2.0f),
				(int) (res.y + (nf->header.dim[2] / 2.0f) - hdim / 2.0f),
				(int) (res.z + (nf->header.dim[3] / 2.0f) - hdim / 2.0f));

			high_value = nf->volume[nf->transformVector3Position(res) ];
			low_value = high_value;
		}
		else { //position is out of dataset range
			high_value = 0.0f;
			low_value = high_value;
		}
		/*
		if (res.x >= 0.0f && res.x < nf->header.dim[1] &&
			res.y >= 0.0f && res.y < nf->header.dim[2] &&
			res.z >= 0.0f && res.z < nf->header.dim[3])
		{
			high_value = nf->volume[nf->transformVector3Position(res)];
			low_value = high_value;
		}
		else { //position is out of dataset range
			high_value = 0.0f;
			low_value = high_value;
		}*/

	}
}

/*
* No tranformations are applied so be carefull, be sure to transform point to local cordinates before using function
*/
bool Node::isInside(glm::vec3 p) {
	return (p.x >= lower.x &&
		p.y >= lower.y &&
		p.z >= lower.z &&
		p.x < upper.x&&
		p.y < upper.y&&
		p.z < upper.z);
}

bool Node::isLeaf() {
	return branches == nullptr;
}

float Node::searchPointGetIntensity(glm::vec3 point) {

	float res = 0.0f;

	if (isInside(point)) {
		if (high_value == low_value) { // early stop
			res = high_value;
		}
		else {
			for (int i = 0; i < 8; i++) {
				float aux = this->branches[i][0].searchPointGetIntensity(point);
				if (aux > res)
					res = aux;
			}
		}
	}
	return res;
}

Node::~Node() {
	if (!isLeaf())
		free(branches);
}
