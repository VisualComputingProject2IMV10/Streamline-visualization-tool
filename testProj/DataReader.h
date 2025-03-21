#pragma once

#include <vector>

int readData(float*& output, short& dimX, short& dimY, short& dimZ);

void printSlice(float* data, int slice, int x, int y, int z);