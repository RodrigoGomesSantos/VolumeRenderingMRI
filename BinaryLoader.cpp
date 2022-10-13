// reading an entire nifti2 binary file

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>

//NIFTI 2 inclusion
#include "nifti2.h"

#include "BinaryLoader.h"


NiftiFile::NiftiFile(std::string filename)
{
    this->fileName = filename;
    loadFileToMem();
    niftiType = 0;
    if (header.magic == NULL) {
        std::cerr << "WARNING: Header not populated!" << std::endl;
    }
    niftiType = header.sizeof_hdr;
}

void NiftiFile::displayNIFTI2Header() {
    std::cout << "SIZEOF_HDR: " << header.sizeof_hdr << std::endl;
    std::cout << "MAGIC: " << header.magic << std::endl;
    std::cout << "DATATYPE: " << header.datatype << std::endl;
    std::cout << "BITPIX: " << header.bitpix << std::endl;

    std::cout << "DIM:";
    int arrSize = sizeof(header.dim) / sizeof(header.dim[0]);
    for (int i = 0; i < (arrSize); i++) {
        std::cout << "\t" << header.dim[i];
    }
    std::cout << std::endl;

    std::cout << "INTENT_P1: " << header.intent_p1 << std::endl;
    std::cout << "INTENT_P2: " << header.intent_p2 << std::endl;
    std::cout << "INTENT_P3: " << header.intent_p3 << std::endl;

    std::cout << "PIXDIM:";
    arrSize = sizeof(header.pixdim) / sizeof(header.pixdim[0]);
    for (int i = 0; i < (arrSize); i++) {
        std::cout << "\t" << header.pixdim[i];
    }
    std::cout << std::endl;

    std::cout << "VOX_OFFSET: " << header.vox_offset << std::endl;

    std::cout << "SCL_SLOPE: " << header.scl_slope << std::endl;
    std::cout << "SCL_INTER: " << header.scl_inter << std::endl;

    std::cout << "CAL_MAX: " << header.cal_max << std::endl;
    std::cout << "CAL_MIN: " << header.cal_min << std::endl;

    std::cout << "SLICE_DURATION: " << header.slice_duration << std::endl;
    std::cout << "TOFFSET: " << header.toffset << std::endl;
    std::cout << "SLICE START: " << header.slice_start << std::endl;
    std::cout << "SLICE END: " << header.slice_end << std::endl;
    std::cout << "DESCRIP: " << header.descrip << std::endl;
    std::cout << "AUX FILE:" << header.aux_file << std::endl;

    std::cout << "QFORM_CODE:" << header.qform_code << std::endl;
    std::cout << "SFORM_CODE:" << header.sform_code << std::endl;

    std::cout << "QUATERN_B:" << header.quatern_b << std::endl;
    std::cout << "QUATERN_C:" << header.quatern_c << std::endl;
    std::cout << "QUATERN_D:" << header.quatern_d << std::endl;

    std::cout << "QOFFSET_X:" << header.qoffset_x << std::endl;
    std::cout << "QOFFSET_Y:" << header.qoffset_y << std::endl;
    std::cout << "QOFFSET_Z:" << header.qoffset_z << std::endl;

    std::cout << "SROW_X:\t" << header.srow_x[0] << "\t" << header.srow_x[1] << "\t" << header.srow_x[2] << "\t" << header.srow_x[3] << std::endl;
    std::cout << "SROW_Y:\t" << header.srow_y[0] << "\t" << header.srow_y[1] << "\t" << header.srow_y[2] << "\t" << header.srow_y[3] << std::endl;
    std::cout << "SROW_Z:\t" << header.srow_z[0] << "\t" << header.srow_z[1] << "\t" << header.srow_z[2] << "\t" << header.srow_z[3] << std::endl;

    std::cout << "SLICE_CODE: " << header.slice_code << std::endl;
    std::cout << "XYZT_UNITS: " << header.xyzt_units << std::endl;
    std::cout << "INTENT_CODE: " << header.intent_code << std::endl;
    std::cout << "INTENT_NAME: " << header.intent_name << std::endl;
    std::cout << "DIM_INFO: " << header.dim_info << std::endl;
    std::cout << "UNUSED_STR: " << header.unused_str << std::endl;
}

/**
* search for the volume index
* only works for data with dim[0] == 3
* v - 3 cordiante vector
* returns - the corresponding index
*/
int NiftiFile::transformVector3Position(glm::vec3 v) {
    // x * (Y * Z) + y * (Z) + z
    return v.x * header.dim[2] * header.dim[3] + v.y * header.dim[3] + v.z;

}

int NiftiFile::loadFileToMem() {

    std::streampos size;

    std::ifstream file(fileName /*"avg152T1_LR_nifti2.nii"*/, std::ios::in | std::ios::binary  | std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();

        std::cout << "FILE SIZE = " << size << std::endl;

        //NIFTI2 HEADER
        file.seekg(0, std::ios::beg);
        file.read((char*)&header, sizeof(nifti_2_header));
        if (header.sizeof_hdr != 540) {

            std::cerr << "file isn't a nifti 2 format, maybe file read in wrong endianess!: " << header.sizeof_hdr << std::endl;
            return 1;
        }


        //NIFTI2 DATA
        //nifti2 file data starts after byte 544

        /*|------------------|
          |GENERATE STRUCTURE|
          |------------------|*/

        //its known for this particular file that its dimensions are 3 (x,y,z respectively)

          
        int vsize = header.dim[1]*header.dim[2]*header.dim[3];
        volume = new float[vsize];

        //start reading from byte offset, for nifti2 is ususally 544, https://brainder.org/2015/04/03/the-nifti-2-file-format/
        file.seekg (header.vox_offset, std::ios::beg); 
          
        std::cout << "\nCONTENT START"<< std::endl;
       
          
        std::cout << "File good: " << file.good() << std::endl;

        file.read((char*) volume, vsize * sizeof(float));
          
        std::cout << "File end: " << file.eof() << std::endl;
          
        std::cout << "\nCONTENT END" << std::endl;

        //CLOSE FILE
        file.close();
        std::cout << "the entire file content is in memory" << std::endl;
    }
    else std::cout << "Unable to open file";
    return 0;
}

void NiftiFile::iterate() {
    for (int i = 0; i < header.dim[1]; i++) {
        for (int j = 0; j < header.dim[2]; j++) {
            for (int k = 0; k < header.dim[3]; k++) {
                volume[transformVector3Position(glm::vec3(i, j, k))];
            }
        }
    }
}