#ifndef CONSTANTS_H
#define CONSTANTS_H

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

//Paths
const char* const BRAIN_SCALAR_PATH = "data/brain-map.nii";
const char* const BRAIN_VECTOR_PATH = "data/brain-vectors.nii";
const char* const BRAIN_TENSORS_PATH = "data/brain-tensors.nii";
const char* const TOY_SCALAR_PATH = "data/toy-map.nii";
const char* const TOY_VECTOR_PATH = "data/toy-evec.nii";

const char* const BRAIN_DATASET = "Brain dataset";
const char* const TOY_DATASET = "Toy dataset";

const bool USE_SMOOTH_BACKGROUND = false;

const float NEAR_CAM_PLANE = 0.01f;
const float FAR_CAM_PLANE = 1000.0f;

const short AXIS_X = 0;
const short AXIS_Y = 1;
const short AXIS_Z = 2;

#endif