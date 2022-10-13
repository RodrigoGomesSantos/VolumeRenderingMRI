#pragma once

#include <string>
#include "nifti2.h"


class NiftiFile {

private:
	std::string fileName;
	
	int niftiType;
	
public:
	nifti_2_header header;
	float* volume;
	NiftiFile(std::string filename);
	int transformVector3Position(glm::vec3 v);
	void displayNIFTI2Header();
	int loadFileToMem();
	void iterate();

};
