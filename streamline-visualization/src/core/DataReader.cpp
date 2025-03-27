#include "../include/DataReader.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include "../extra/nifti1.h"

int readData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ) {
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

void printSlice(float* data, int slice, int dimX, int dimY, int dimZ) {
    if (!data || slice < 0 || slice >= dimZ) {
        std::cerr << "Error: Invalid parameters for printSlice" << std::endl;
        return;
    }

    std::cout << "Slice " << slice << " data:" << std::endl;
    for (int y = 0; y < dimY; y++) {
        for (int x = 0; x < dimX; x++) {
            int index = slice * dimX * dimY + y * dimX + x;
            std::cout << data[index] << " ";
        }
        std::cout << std::endl;
    }
}

int readTensorData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ) {
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
    int dimT = header.dim[4];  // Time dimension (should be 6 for tensor data)

    if (dimT != 6) {
        std::cerr << "Warning: Expected 6 tensor components, found " << dimT << std::endl;
    }

    // For tensor data, we need 6 components per voxel (D11, D22, D33, D12, D13, D23)
    int numVoxels = dimX * dimY * dimZ;
    int numComponents = dimT; // Usually 6 for tensor data

    // Allocate memory for data
    data = new float[numVoxels * numComponents];

    // Skip to the data section if there's an extended header
    if (header.vox_offset > sizeof(nifti_1_header)) {
        file.seekg(header.vox_offset, std::ios::beg);
    }

    // Read the data
    file.read(reinterpret_cast<char*>(data), numVoxels * numComponents * sizeof(float));

    if (!file.good()) {
        std::cerr << "Error: Failed to read tensor data" << std::endl;
        delete[] data;
        data = nullptr;
        file.close();
        return EXIT_FAILURE;
    }

    file.close();
    std::cout << "Successfully read tensor data: " << dimX << "x" << dimY << "x" << dimZ
              << " with " << numComponents << " components per voxel" << std::endl;

    return EXIT_SUCCESS;
}

int readVectorData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ) {
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

    // Skip to the data section if there's an extended header
    if (header.vox_offset > sizeof(nifti_1_header)) {
        file.seekg(header.vox_offset, std::ios::beg);
    }

    // Read the data
    file.read(reinterpret_cast<char*>(data), numVoxels * numComponents * sizeof(float));

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