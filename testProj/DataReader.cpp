
#include <iostream>
#include <fstream> 
#include "nifti1.h"

#include "DataReader.h"

/* Debug function for printing a slice of data */
void printSlice(float* data, int slice, int x, int y, int z)
{
	for (int i = 0; i < x; i++) {
		for (int j = 0; j < y; j++) {
			printf("%.1f ", data[slice + z * (j + y * i)]);
		}
		printf("\n");
	}
}

/* Read data from a (hardcoded) nifti file */
int readData(float*& output, short &dimX, short &dimY, short &dimZ)
{
	using namespace std;

	ifstream file("data/toy-map.nii", ios::binary); // Open file in binary mode
	if (!file) {
		std::cerr << "Error opening file\n";
		return EXIT_FAILURE;
	}

	nifti_1_header header;
	file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

	// Print some header info
	if (false)
	{
		cout << "Output nifti data header" << "\n";

		cout << "Data type" << "\n";
		cout << header.datatype << "\n";

		cout << "Dimensions" << "\n";
		for (short dimension : header.dim)
		{
			cout << dimension << "\n";
		}
	}

	const short d1 = header.dim[1];
	const short d2 = header.dim[2];
	const short d3 = header.dim[3];

	float* data = new float[d1 * d2 * d3];

	for (int i = 0; i < d1; i++) {
		for (int j = 0; j < d2; j++) {
			for (int k = 0; k < d3; k++) {
				float value;
				file.read((char*)&value, sizeof(value));
				data[k + d3*(j + d2*i)] = value;
			}
		}
	}
	file.close();

	output = data;

	dimX = d1;
	dimY = d2;
	dimZ = d3;

	return EXIT_SUCCESS;
}