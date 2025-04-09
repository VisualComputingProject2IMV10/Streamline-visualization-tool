#include "../include/DataReader.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include "../extra/nifti1.h"

int readData(const char* filename, float*& data, int& dimX, int& dimY, int& dimZ) {
    // Open NIFTI file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return EXIT_FAILURE;
    }

    // Read NIFTI header
    nifti_1_header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

    if (!file.good()) {
        std::cerr << "Error: Failed to read NIFTI header" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Validate NIFTI format
    if (strncmp(header.magic, "n+1", 3) != 0 && strncmp(header.magic, "ni1", 3) != 0) {
        std::cerr << "Error: Not a valid NIFTI file" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Extract dimensions
    dimX = header.dim[1];
    dimY = header.dim[2];
    dimZ = header.dim[3];

    // For scalar data, we need 1 value per voxel
    int numVoxels = dimX * dimY * dimZ;

    // Allocate memory for data
    data = new float[numVoxels];

    // Skip to the data section if there's an extended header
    if (header.vox_offset > sizeof(nifti_1_header)) {
        file.seekg(header.vox_offset, std::ios::beg);
    }

    // Read the data
    file.read(reinterpret_cast<char*>(data), numVoxels * sizeof(float));

    if (!file.good()) {
        std::cerr << "Error: Failed to read scalar data" << std::endl;
        delete[] data;
        data = nullptr;
        file.close();
        return EXIT_FAILURE;
    }

    file.close();
    std::cout << "Successfully read scalar data: " << dimX << "x" << dimY << "x" << dimZ << std::endl;

    return EXIT_SUCCESS;
}

int readTensorData(const char* filename, float*& data, int& dimX, int& dimY, int& dimZ) {
    // Open NIFTI file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return EXIT_FAILURE;
    }

    // Read NIFTI header
    nifti_1_header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

    if (!file.good()) 
    {
        std::cerr << "Error: Failed to read NIFTI header" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Validate NIFTI format
    if (strncmp(header.magic, "n+1", 3) != 0 && strncmp(header.magic, "ni1", 3) != 0) 
    {
        std::cerr << "Error: Not a valid NIFTI file" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Extract dimensions
    dimX = header.dim[1];
    dimY = header.dim[2];
    dimZ = header.dim[3];

    int numVoxels = dimX * dimY * dimZ;
    int numComponents = 6; // Symetrical tensor so 6 components

    if (header.dim[4] != numComponents)
    {
        std::cerr << "Error: invalid number of tensor components: " << header.dim[4] << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Allocate memory for data (6 components per voxel)
    data = new float[numVoxels * numComponents];


    for (int v = 0; v < numComponents; v++)
    {
        for (int k = 0; k < dimZ; k++)
        {
            for (int j = 0; j < dimY; j++)
            {
                for (int i = 0; i < dimX; i++)
                {
                    int index = i * (dimZ * dimY * numComponents) + j * (dimZ * numComponents) + k * numComponents + v;
                    float value;
                    file.read((char*)&value, sizeof(value));

                    //error compensation for near zero values
                    if (value != 0.0f && (value <= 0.00001f && value >= -0.00001f)) value = 0.0f;
                    if (isnan(value)) value = 0.0f; //default to zero to make sure we keep proper vectors

                    data[index] = value;
                }
            }
        }
    }

    if (!file.good()) {
        std::cerr << "Error: Failed to read tensor data" << std::endl;
        delete[] data;
        data = nullptr;
        file.close();
        return EXIT_FAILURE;
    }

    file.close();
    std::cout << "Successfully read tensor data: " << dimX << "x" << dimY << "x" << dimZ << std::endl;

    return EXIT_SUCCESS;
}

int readVectorData(const char* filename, float*& data, int& dimX, int& dimY, int& dimZ) {
    // Open NIFTI file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return EXIT_FAILURE;
    }

    // Read NIFTI header
    nifti_1_header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

    if (!file.good()) {
        std::cerr << "Error: Failed to read NIFTI header" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Validate NIFTI format
    if (strncmp(header.magic, "n+1", 3) != 0 && strncmp(header.magic, "ni1", 3) != 0) {
        std::cerr << "Error: Not a valid NIFTI file" << std::endl;
        file.close();
        return EXIT_FAILURE;
    }

    // Extract dimensions
    dimX = header.dim[1];
    dimY = header.dim[2];
    dimZ = header.dim[3];

    // For vector data, we need 3 components per voxel (x, y, z)
    int numVoxels = dimX * dimY * dimZ;
    int numComponents = 3; // Vector field (x, y, z components)

    // Allocate memory for data (3 components per voxel)
    data = new float[numVoxels * numComponents];
   
    for (int v = 0; v < numComponents; v++)
    {
        for (int k = 0; k < dimZ; k++) 
        {
            for (int j = 0; j < dimY; j++) 
            {
                for (int i = 0; i < dimX; i++) 
                {
                    int index = i * (dimZ * dimY * numComponents) + j * (dimZ * numComponents) + k * numComponents + v;
                    float value;
                    file.read((char*)&value, sizeof(value));
                    data[index] = value;
                }
            }
        }
    }

    if (!file.good()) {
        std::cerr << "Error: Failed to read vector data" << std::endl;
        delete[] data;
        data = nullptr;
        file.close();
        return EXIT_FAILURE;
    }

    file.close();
    std::cout << "Successfully read vector data: " << dimX << "x" << dimY << "x" << dimZ << std::endl;

    return EXIT_SUCCESS;
}