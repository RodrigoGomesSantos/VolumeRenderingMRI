

#include <iostream>
#include <fstream>
#include <string>



void TransferFunction() {

	//receive a file and then parces its contents to fill a multivariate array

    char* charfile;

    std::streampos size;
    std::ifstream file("TransferFunction.txt", std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();

        std::cout << "FILE SIZE = " << size << std::endl;

        charfile = (char*)malloc(sizeof(char));

        //NIFTI2 HEADER
        file.seekg(0, std::ios::beg);
        file.read(charfile, size);
        
        //CLOSE FILE
        file.close();
        std::cout << "the entire file content is in memory" << std::endl;

        free(charfile);
    }
    else std::cout << "Unable to open file";


}