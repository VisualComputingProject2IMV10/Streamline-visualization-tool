#pragma once

#include <vector>

/**
 * @file DataReader.h
 * @brief Utility functions for reading NIFTI volumetric data files
 *
 * This file contains functions for reading various types of volumetric data
 * from NIFTI format files (.nii, .nii.gz), including scalar, vector, and tensor data.
 */

/**
 * @brief Read scalar data from a NIFTI file
 *
 * @param filename Path to the NIFTI file
 * @param data Output parameter to store the loaded data (caller must delete[])
 * @param dimX Output parameter for X dimension
 * @param dimY Output parameter for Y dimension
 * @param dimZ Output parameter for Z dimension
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int readData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ);

/**
 * @brief Print a slice of 3D data to console (for debugging)
 *
 * @param data The 3D data array
 * @param slice Slice index
 * @param x X dimension
 * @param y Y dimension
 * @param z Z dimension
 */
void printSlice(float* data, int slice, int x, int y, int z);

/**
 * @brief Read vector field data from a NIFTI file
 *
 * @param filename Path to the NIFTI file
 * @param data Output parameter to store the loaded data (3 components per voxel, caller must delete[])
 * @param dimX Output parameter for X dimension
 * @param dimY Output parameter for Y dimension
 * @param dimZ Output parameter for Z dimension
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int readVectorData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ);

/**
 * @brief Read diffusion tensor data from a NIFTI file
 *
 * @param filename Path to the NIFTI file
 * @param data Output parameter to store the loaded data (6 components per voxel, caller must delete[])
 * @param dimX Output parameter for X dimension
 * @param dimY Output parameter for Y dimension
 * @param dimZ Output parameter for Z dimension
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int readTensorData(const char* filename, float*& data, short& dimX, short& dimY, short& dimZ);