#include "../include/VectorField.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include "../extra/nifti1.h"
#include "../include/DataReader.h"

VectorField::VectorField(const char* filename) {
    float* vectorData;
    int dimX, dimY, dimZ;

    // Load vector data from file using the data reader
    if (readVectorData(filename, vectorData, dimX, dimY, dimZ) != EXIT_SUCCESS) {
        std::cerr << "Failed to load vector field from " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    // Store field properties
    this->data = vectorData;
    this->dimX = dimX;
    this->dimY = dimY;
    this->dimZ = dimZ;

    this->zeroMask = calculateZeroMask();

    std::cout << "Loaded vector field: " << dimX << "x" << dimY << "x" << dimZ << std::endl;
}

VectorField::VectorField(float* tensorField, int dimX, int dimY, int dimZ)
{
    this->dimX = dimX;
    this->dimY = dimY;
    this->dimZ = dimZ;

    //initialize the vector field
    this->data = new float[dimX * dimY * dimZ * 3];

    std::cout << "Start processing tensors" << std::endl;

    //fill the vectorfield with the tensor eigenvectors
    for (int x = 0; x < this->dimX; x++)
    {
        for (int y = 0; y < this->dimY; y++)
        {
            for (int z = 0; z < this->dimZ; z++)
            {
                Eigen::Vector3f eigenVector = getMajorEigenVector(tensorField, x, y, z);

                // Calculate index into data array (3 components per voxel)
                int index = 3 * (z + dimZ * (y + dimY * x));
                this->data[index + 0] = eigenVector(0);
                this->data[index + 1] = eigenVector(1);
                this->data[index + 2] = eigenVector(2);
            }
        }
    }

    this->zeroMask = calculateZeroMask();

    std::cout << "Initialized vector field from tensor field" << std::endl;
}

Eigen::Vector3f VectorField::getMajorEigenVector(float* tensorField, int x, int y, int z)
{
    int index = 6 * (z + this->dimZ * (y + this->dimY * x));

    //load the tensor data
    float t11 = tensorField[index + 0];
    float t22 = tensorField[index + 1];
    float t33 = tensorField[index + 2];
    float t12 = tensorField[index + 3];
    float t13 = tensorField[index + 4];
    float t23 = tensorField[index + 5];

    if (t11 == 0.0f && t22 == 0.0f && t33 == 0.0f && t12 == 0.0f && t13 == 0.0f && t23 == 0.0f)
    {
        Eigen::Vector3f v = { 0.0f,0.0f,0.0f };
        return v;
    }

    Eigen::Matrix3f tensor;
    tensor << 
        t11, t12, t13,
        t12, t22, t23,
        t13, t23, t33;

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(tensor);
    if (eigensolver.info() != Eigen::Success) 
    {
        //make sure the solver converges
        std::cout << "Error while calculating eigenvalues" << std::endl;
        std::cout << tensor << "\n\n";
        std::cout << t11 << ", " << t22 << ", " << t33 << ", " << t12 << ", " << t13 << ", " << t23 << std::endl;
    }

    return eigensolver.eigenvectors().col(2); //the third eigenvector corresponds to the largest eigenvalue
}

VectorField::~VectorField() {
    // Free allocated memory
    delete[] data;
}

void VectorField::getVector(int x, int y, int z, float& vx, float& vy, float& vz) const {
    // Bounds checking
    if (x < 0 || x >= dimX || y < 0 || y >= dimY || z < 0 || z >= dimZ) {
        // Return zero vector if out of bounds
        vx = vy = vz = 0.0f;
        return;
    }

    // Calculate index into data array (3 components per voxel)
    int index = 3 * (z + dimZ * (y + dimY * x));

    // Extract vector components
    vx = flipX ? -1.0f * data[index] : data[index];
    vy = flipY ? -1.0f * data[index + 1] : data[index + 1];
    vz = flipZ ? -1.0f * data[index + 2] : data[index + 2];
}

void VectorField::interpolateVector(float x, float y, float z, float& vx, float& vy, float& vz) const {
    // If outside bounds, return zero vector
    if (!isInBounds(x, y, z)) {
        std::cout << "interpolated vector out of bounds" << std::endl;
        vx = vy = vz = 0.0f;
        return;
    }

    if (SIMPLE_INTERPOLATION)
    {
        int x0 = std::roundf(x);
        int y0 = std::roundf(y);
        int z0 = std::roundf(z);

        this->getVector(x0, y0, z0, vx, vy, vz);
        return;
    }

    // Calculate indices of surrounding voxels
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int z0 = static_cast<int>(z);

    // Ensure indices are within bounds for interpolation
    x0 = std::max(0, std::min(x0, dimX-2));
    y0 = std::max(0, std::min(y0, dimY-2));
    z0 = std::max(0, std::min(z0, dimZ-2));

    int x1 = std::min(x0 + 1, dimX-1);
    int y1 = std::min(y0 + 1, dimY-1);
    int z1 = std::min(z0 + 1, dimZ-1);

    // Calculate interpolation weights
    float wx = x - x0;
    float wy = y - y0;
    float wz = z - z0;

    // Clamp weights to [0,1] range
    wx = std::max(0.0f, std::min(1.0f, wx));
    wy = std::max(0.0f, std::min(1.0f, wy));
    wz = std::max(0.0f, std::min(1.0f, wz));

    // Get vectors at the 8 surrounding voxels
    float v000x, v000y, v000z;
    float v001x, v001y, v001z;
    float v010x, v010y, v010z;
    float v011x, v011y, v011z;
    float v100x, v100y, v100z;
    float v101x, v101y, v101z;
    float v110x, v110y, v110z;
    float v111x, v111y, v111z;

    getVector(x0, y0, z0, v000x, v000y, v000z);
    getVector(x0, y0, z1, v001x, v001y, v001z);
    getVector(x0, y1, z0, v010x, v010y, v010z);
    getVector(x0, y1, z1, v011x, v011y, v011z);
    getVector(x1, y0, z0, v100x, v100y, v100z);
    getVector(x1, y0, z1, v101x, v101y, v101z);
    getVector(x1, y1, z0, v110x, v110y, v110z);
    getVector(x1, y1, z1, v111x, v111y, v111z);

    // Trilinear interpolation for each vector component
    vx = (1-wx)*(1-wy)*(1-wz)*v000x + (1-wx)*(1-wy)*wz*v001x +
         (1-wx)*wy*(1-wz)*v010x + (1-wx)*wy*wz*v011x +
         wx*(1-wy)*(1-wz)*v100x + wx*(1-wy)*wz*v101x +
         wx*wy*(1-wz)*v110x + wx*wy*wz*v111x;

    vy = (1-wx)*(1-wy)*(1-wz)*v000y + (1-wx)*(1-wy)*wz*v001y +
         (1-wx)*wy*(1-wz)*v010y + (1-wx)*wy*wz*v011y +
         wx*(1-wy)*(1-wz)*v100y + wx*(1-wy)*wz*v101y +
         wx*wy*(1-wz)*v110y + wx*wy*wz*v111y;

    vz = (1-wx)*(1-wy)*(1-wz)*v000z + (1-wx)*(1-wy)*wz*v001z +
         (1-wx)*wy*(1-wz)*v010z + (1-wx)*wy*wz*v011z +
         wx*(1-wy)*(1-wz)*v100z + wx*(1-wy)*wz*v101z +
         wx*wy*(1-wz)*v110z + wx*wy*wz*v111z;
}

bool VectorField::isInBounds(float x, float y, float z) const {
    // Check if point is within the field bounds (allowing for interpolation)
    return (x >= 0.0f && x <= dimX-1.0f &&
            y >= 0.0f && y <= dimY-1.0f &&
            z >= 0.0f && z <= dimZ-1.0f);
}

bool* VectorField::getZeroMask(int dimX, int dimY, int dimZ)
{
    //check if dimensions match
    if (this->dimX != dimX || this->dimY != dimY || this->dimZ != dimZ)
    {
        throw std::length_error("Dimensions should match up with the vector field");
    }

    return this->zeroMask;
}

bool* VectorField::calculateZeroMask()
{
    bool* mask = new bool[this->dimX * this->dimY * this->dimZ];
    int index;
    float vx, vy, vz;
    for (size_t x = 0; x < this->dimX; x++)
    {
        for (size_t y = 0; y < this->dimY; y++)
        {
            for (size_t z = 0; z < this->dimZ; z++)
            {
                index = x + y * dimX + z * dimX * dimY;
                this->getVector(x, y, z, vx, vy, vz);

                //if we get a zero vector, we are outside the image
                if (vx == 0.0f && vy == 0.0f && vz == 0.0f)
                {
                    mask[index] = false;
                }
                else
                {
                    mask[index] = true;
                }
            }
        }
    }
    return mask;
}