#pragma once

#include <string>
#include <Eigen/Dense>

/**
 * @class VectorField
 * @brief Represents a 3D vector field
 *
 * This class handles loading, storage, and querying of 3D vector field data,
 * supporting both direct access and interpolation of vector values.
 */
class VectorField {
public:

    const bool SIMPLE_INTERPOLATION = true;

    /**
     * @brief Constructor that loads vector field from file
     * @param filename Path to the vector field file (NIFTI format)
     */
    VectorField(const char* filename);

    /**
     * Construct vector field from the eigenvectors of the tensor field.
     */
    VectorField(float* tensorField, int dimX, int dimY, int dimZ);

    /**
     * @brief Destructor - frees allocated memory
     */
    ~VectorField();

    /**
     * @brief Get vector value at integer grid indices
     * @param x X index
     * @param y Y index
     * @param z Z index
     * @param vx Output X component of vector
     * @param vy Output Y component of vector
     * @param vz Output Z component of vector
     */
    void getVector(int x, int y, int z, float& vx, float& vy, float& vz) const;

    /**
     * @brief Get interpolated vector at floating-point coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param vx Output X component of vector
     * @param vy Output Y component of vector
     * @param vz Output Z component of vector
     */
    void interpolateVector(float x, float y, float z, float& vx, float& vy, float& vz) const;

    /**
     * @brief Check if a point is within the field bounds
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return True if point is in bounds, false otherwise
     */
    bool isInBounds(float x, float y, float z) const;

    // Accessor methods
    bool* getZeroMask(int dimX, int dimY, int dimZ);


    //some nifti files have the axis flipped
    bool flipX = false;
    bool flipY = false;
    bool flipZ = false;

    short dimX, dimY, dimZ;  ///< Dimensions of the vector field

private:
    float* data;         ///< Vector data (3 components per voxel)
    bool* zeroMask; ///Mask of zero vectors

    bool* calculateZeroMask();

    Eigen::Vector3f getMajorEigenVector(float* tensorField, int x, int y, int z);
};