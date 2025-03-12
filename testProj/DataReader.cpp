
#include <iostream>
#include <fstream> 
#include <ranges>
#include "nifti1.h"

#include "DataReader.h"

int readData(float*& output, short &dimX, short &dimY, short &dimZ)
{
	using namespace std;

	const bool printHeader = false;

	ifstream file("data/toy-map.nii", ios::binary); // Open file in binary mode
	if (!file) {
		std::cerr << "Error opening file\n";
		return EXIT_FAILURE;
	}

	nifti_1_header header;
	file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

	// TEST output
	if (printHeader)
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

	/*vector<vector<vector<float>>> data(d1, vector<vector<float>>
		(d2, vector<float>(d3, 0)));*/
	float* data = new float[d1 * d2 * d3];

	for (int i = 0; i < d1; ++i) {
		for (int j = 0; j < d2; ++j) {
			for (int k = 0; k < d3; ++k) {
				float value;
				file.read((char*)&value, sizeof(value));
				data[i + i*j + i*j*k] = value;
				//printf("%f ", value);
			}
			//printf("\n");
		}
		//printf("\n");
	}

	output = data;
	//cout << "Size of data: " << data.size() << ", " << data[0].size() << ", " << data[0][0].size() << endl;

	file.close();

	return EXIT_SUCCESS;
}

int readData(std::vector<std::vector<std::vector<float>>> &output)
{
	using namespace std;

	const bool printHeader = false;

	ifstream file("data/toy-map.nii", ios::binary); // Open file in binary mode
	if (!file) {
		std::cerr << "Error opening file\n";
		return EXIT_FAILURE;
	}

	nifti_1_header header;
	file.read(reinterpret_cast<char*>(&header), sizeof(nifti_1_header));

	// TEST output
	if (printHeader)
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

	vector<vector<vector<float>>> data(d1, vector<vector<float>>
		(d2, vector<float>(d3, 0)));

	for (int i = 0; i < d1; ++i) {
		for (int j = 0; j < d2; ++j) {
			for (int k = 0; k < d3; ++k) {
				float value;
				file.read((char*) &value, sizeof(value));
				data[i][j][k] = value;
				//printf("%f ", value);
			}
			//printf("\n");
		}
		//printf("\n");
	}

	output = data;
	//cout << "Size of data: " << data.size() << ", " << data[0].size() << ", " << data[0][0].size() << endl;

	file.close();

	return EXIT_SUCCESS;
}