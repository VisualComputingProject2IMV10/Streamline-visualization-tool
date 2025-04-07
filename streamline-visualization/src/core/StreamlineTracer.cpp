#include "../include/StreamlineTracer.h"
#include <cmath>
#include <iostream>
#include <random>

// External function declaration for scalar data sampling (implemented in Source.cpp)
extern float sampleScalarData(float x, float y, float z);

StreamlineTracer::StreamlineTracer(VectorField* field, float step, int steps, float maxLen, float maxAngle)
    : vectorField(field), stepSize(step), maxSteps(steps),
      maxLength(maxLen), maxAngle(maxAngle) {
    // Validate input parameters
    if (!field) {
        std::cerr << "Error: Null vector field provided to StreamlineTracer" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->zeroMask = field->getZeroMask(field->getDimX(), field->getDimY(), field->getDimZ()); //for quicker access
}

std::vector<Point3D> StreamlineTracer::generateSliceGridSeeds(int currentSliceX, int currentSliceY, int currentSliceZ, int axis)
{
    std::cout << "hello world" << std::endl;
    std::vector<Point3D> seeds;

    if (!vectorField) 
    {
        std::cerr << "Error: Vector field is nullptr in generateUnifiedBrainSeeds" << std::endl;
        return seeds;
    }

    if (!(axis == AXIS_X || axis == AXIS_Y || axis == AXIS_Z))
    {
        std::cerr << "Error: undefined axis" << std::endl;
        return seeds;
    }

    int dimX = vectorField->getDimX();
    int dimY = vectorField->getDimY();
    int dimZ = vectorField->getDimZ();

    if (currentSliceX < 0 || currentSliceX >= dimX || currentSliceY < 0 || currentSliceY >= dimY || currentSliceZ < 0 || currentSliceZ >= dimZ)
    {
        std::cerr << "Error: slice out of bounds" << std::endl;
        return seeds;
    }

    std::cout << "Generating simple grid slice seeds" << std::endl;

    //preallocate max memory needed for seeds
    if      (axis == AXIS_X) seeds.reserve(dimY * dimZ);
    else if (axis == AXIS_Y) seeds.reserve(dimX * dimZ);
    else if (axis == AXIS_Z) seeds.reserve(dimX * dimY);
    std::cout << axis << std::endl;
    if (axis == AXIS_X)
    {
        for (int y = 0; y < dimY; y++)
        {
            for (int z = 0; z < dimZ; z++)
            {
                if (this->zeroMask[currentSliceX + y * dimX + z * dimX * dimY])
                {
                    seeds.push_back(Point3D(currentSliceX, y, z));
                }
            }
        }
    }
    else if (axis == AXIS_Y)
    {
        for (int x = 0; x < dimX; x++)
        {
            for (int z = 0; z < dimZ; z++)
            {
                if (this->zeroMask[x + currentSliceY * dimX + z * dimX * dimY])
                {
                    seeds.push_back(Point3D(x, currentSliceY, z));
                }
            }
        }
    }
    else if (axis == AXIS_Z)
    {
        for (int x = 0; x < dimX; x++)
        {
            for (int y = 0; y < dimY; y++)
            {
                if (this->zeroMask[x + y * dimX + currentSliceZ * dimX * dimY])
                {
                    seeds.push_back(Point3D(x, y, currentSliceZ));
                }
            }
        }
    }
    seeds.shrink_to_fit();//free the unused memory

    std::cout << "Sampled " << seeds.size() << " seed points" << std::endl;
    return seeds;
}

std::vector<Point3D> StreamlineTracer::generateMouseSeeds(int sliceX, int sliceY, int sliceZ, int axis, glm::vec3 seedLoc, float seedRadius, float density)
{
    std::vector<Point3D> seeds;
    
    if (!vectorField) {
        std::cerr << "Error: Vector field is nullptr in generateUnifiedBrainSeeds" << std::endl;
        return seeds;
    }

    if (!(axis == AXIS_X || axis == AXIS_Y || axis == AXIS_Z))
    {
        std::cerr << "Error: undefined axis" << std::endl;
        return seeds;
    }

    int dimX = vectorField->getDimX();
    int dimY = vectorField->getDimY();
    int dimZ = vectorField->getDimZ();

    if (sliceX < 0 || sliceX >= dimX || sliceY < 0 || sliceY >= dimY || sliceZ < 0 || sliceZ >= dimZ)
    {
        std::cerr << "Error: slice out of bounds" << std::endl;
        return seeds;
    }

    std::cout << "Generating mouse seeds" << std::endl;

    if (axis == AXIS_X)
    {
        if (!vectorField->isInBounds(sliceX, seedLoc.y, seedLoc.z)) return seeds;
        if (!inZeroMask(glm::vec3(sliceX, seedLoc.y, seedLoc.z))) return seeds;
    }
    else if (axis == AXIS_Y)
    {
        if (!vectorField->isInBounds(seedLoc.x, sliceY, seedLoc.z)) return seeds;
        if (!inZeroMask(glm::vec3(seedLoc.x, sliceY, seedLoc.z))) return seeds;
    }
    else if (axis == AXIS_Z)
    {
        if (!vectorField->isInBounds(seedLoc.x, seedLoc.y, sliceZ)) return seeds;
        if (!inZeroMask(glm::vec3(seedLoc.x, seedLoc.y, sliceZ))) return seeds;
    }
    int maxSeeds = std::roundf(seedRadius) * roundf(density);
    seeds.reserve(maxSeeds); //pre allocate memory
    


    for (size_t i = 0; i < maxSeeds; i++)
    {
        //todo make it spherical
        float r = seedRadius * std::sqrtf((float) std::rand() / RAND_MAX);
        float theta = (float) std::rand() / RAND_MAX * 2 * std::_Pi_val;
        float phi = (float)std::rand() / RAND_MAX * std::_Pi_val;

        glm::vec3 seedPoint = glm::vec3(r * std::cosf(theta) * std::sinf(phi), r * std::sinf(theta) * std::sinf(phi), r * std::cosf(phi));
        
        if (axis == AXIS_X)
        {
            //seedPoint = glm::vec3(sliceX, seedLoc.y + r * std::cosf(theta), seedLoc.z + r * std::sinf(theta));
            seedPoint += glm::vec3(sliceX, seedLoc.y, seedLoc.z);
        }
        else if (axis == AXIS_Y)
        {
            //seedPoint = glm::vec3(seedLoc.x + r * std::cosf(theta), sliceY, seedLoc.z + r * std::sinf(theta));
            seedPoint += glm::vec3(seedLoc.x, sliceY, seedLoc.z);
        }
        else if (axis == AXIS_Z)
        {
            //seedPoint = glm::vec3(seedLoc.x + r * std::cosf(theta), seedLoc.y + r * std::sinf(theta), sliceZ);
            seedPoint += glm::vec3(seedLoc.x, seedLoc.y, sliceZ);
        }

        if (vectorField->isInBounds(seedPoint.x, seedPoint.y, seedPoint.z))
        {
            if (inZeroMask(seedPoint))
            {
                seeds.push_back(Point3D(seedPoint.x, seedPoint.y, seedPoint.z));
            }
        }
    }

    seeds.shrink_to_fit();
    return seeds;

}

std::vector<std::vector<Point3D>> StreamlineTracer::traceVectors(std::vector<Point3D> seeds)
{
    std::vector<std::vector<Point3D>> streamlines;
    streamlines.reserve(seeds.size());  // Pre-allocate memory

    for each(Point3D seed in seeds)
    {
        float vx, vy, vz;
        vectorField->getVector(seed.x, seed.y, seed.z, vx, vy, vz);
        vz = 0;
        int mag = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (mag > 0)
        {
            vx /= mag;
            vy /= mag;
            vz /= mag;
        }
        /*vx *= mag;
        vy *= mag;
        vz *= mag;*/

        //printf("Vector at point (%.1f, %.1f, %.1f) is [%.3f, %.3f, %.3f]\n", seed.x, seed.y, seed.z, vx, vy, vz);

        Point3D head = Point3D(seed.x + vx, seed.y + vy, seed.z + vz);
        Point3D tail = Point3D(seed.x - vx, seed.y - vy, seed.z - vz);
        std::vector<Point3D> vec = { tail, seed, head };
        streamlines.push_back(vec);
    }

    std::cout << streamlines.size() << " streamlines computed" << std::endl;
    return streamlines;
}

Point3D StreamlineTracer::eulerIntegrate1(const Point3D& pos, float step) {
    float vx, vy, vz;
    vectorField->interpolateVector(pos.x, pos.y, pos.z, vx, vy, vz);

    // Normalize the vector
    float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
    if (magnitude > 0) {
        vx /= magnitude;
        vy /= magnitude;
        vz /= magnitude;
    }

    // Euler integration: p(t+h) = p(t) + h*v(t)
    return Point3D(
        pos.x + step * vx,
        pos.y + step * vy,
        pos.z + step * vz
    );
}

Point3D StreamlineTracer::rk4Integrate(const Point3D& pos, float step) {
    // RK4 integration provides higher accuracy than Euler method

    // k1 = v(p)
    float k1x, k1y, k1z;
    vectorField->interpolateVector(pos.x, pos.y, pos.z, k1x, k1y, k1z);

    float mag1 = std::sqrt(k1x*k1x + k1y*k1y + k1z*k1z);
    if (mag1 > 0) {
        k1x /= mag1; 
        k1y /= mag1; 
        k1z /= mag1;
    }

    // k2 = v(p + step/2 * k1)
    float k2x, k2y, k2z;
    vectorField->interpolateVector(
        pos.x + step/2 * k1x,
        pos.y + step/2 * k1y,
        pos.z + step/2 * k1z,
        k2x, k2y, k2z
    );
    float mag2 = std::sqrt(k2x*k2x + k2y*k2y + k2z*k2z);
    if (mag2 > 0) {
        k2x /= mag2; k2y /= mag2; k2z /= mag2;
    }

    // k3 = v(p + step/2 * k2)
    float k3x, k3y, k3z;
    vectorField->interpolateVector(
        pos.x + step/2 * k2x,
        pos.y + step/2 * k2y,
        pos.z + step/2 * k2z,
        k3x, k3y, k3z
    );
    float mag3 = std::sqrt(k3x*k3x + k3y*k3y + k3z*k3z);
    if (mag3 > 0) {
        k3x /= mag3; k3y /= mag3; k3z /= mag3;
    }

    // k4 = v(p + step * k3)
    float k4x, k4y, k4z;
    vectorField->interpolateVector(
        pos.x + step * k3x,
        pos.y + step * k3y,
        pos.z + step * k3z,
        k4x, k4y, k4z
    );
    float mag4 = std::sqrt(k4x*k4x + k4y*k4y + k4z*k4z);
    if (mag4 > 0) {
        k4x /= mag4; k4y /= mag4; k4z /= mag4;
    }

    // RK4 formula: p(t+h) = p(t) + h/6 * (k1 + 2*k2 + 2*k3 + k4)
    return Point3D(
        pos.x + step/6 * (k1x + 2*k2x + 2*k3x + k4x),
        pos.y + step/6 * (k1y + 2*k2y + 2*k3y + k4y),
        pos.z + step/6 * (k1z + 2*k2z + 2*k3z + k4z)
    );
}

std::vector<Point3D> StreamlineTracer::traceStreamline(const Point3D& seed) {
    std::vector<Point3D> streamline;
    // Validate seed point
    if (!vectorField->isInBounds(seed.x, seed.y, seed.z)) {
        /*std::cerr << "Seed point out of bounds: ("
                  << seed.x << ", " << seed.y << ", " << seed.z << ")" << std::endl;*/
        return streamline;
    }

    // Trace in both directions from the seed point
    std::vector<Point3D> forwardPath = traceStreamlineDirection(seed, 1);
    std::vector<Point3D> backwardPath = traceStreamlineDirection(seed, -1);

    // Combine paths (remove duplicate seed point)
    //todo see if this can be optimized
    streamline.clear();
    streamline.insert(streamline.end(), backwardPath.rbegin(), backwardPath.rend());
    streamline.push_back(seed);
    streamline.insert(streamline.end(), forwardPath.begin(), forwardPath.end());

    return streamline;
}

bool StreamlineTracer::inZeroMask(glm::vec3 v)
{
    int x = std::roundf(v.x);
    int y = std::roundf(v.y);
    int z = std::roundf(v.z);
    
    //first sanity check if the coordinates are even in the domain of the mask
    if (x < 0 || x >= this->vectorField->getDimX() || y < 0 || y >= this->vectorField->getDimY() || z < 0 || z >= this->vectorField->getDimZ())
    {
        return false;
    }

    //check the value of the zero mask at the point
    return this->zeroMask[x + y * this->vectorField->getDimX() + z * this->vectorField->getDimX() * this->vectorField->getDimY()];
}

glm::vec3 StreamlineTracer::eulerIntegrate(glm::vec3 pos, float step)
{
    //do first step manually so the loop can check angles more easily
    glm::vec3 vectorAtPos;
    vectorField->interpolateVector(pos.x, pos.y, pos.z, vectorAtPos.x, vectorAtPos.y, vectorAtPos.z);

    // Euler integration: p(t+h) = p(t) + h*v(t)
    glm::vec3 next = pos + step * glm::normalize(vectorAtPos);
    return next;
}

std::vector<Point3D> StreamlineTracer::traceStreamlineDirection(const Point3D& seed, int direction)
{
    std::vector<Point3D> path;
    path.reserve(this->maxSteps); //preallocate max memory for the path

    float totalLength = 0.0f;

    glm::vec3 currentPos = glm::vec3(seed.x, seed.y, seed.z); //convert to glm vector for easier algebra

    glm::vec3 nextPos = eulerIntegrate(currentPos, this->stepSize * direction);

    //check if the seed and next position are valid
    if (inZeroMask(nextPos) || !inZeroMask(currentPos))
    {
        path.push_back(Point3D(nextPos.x, nextPos.y, nextPos.z));
    }
    else
    {
        path.shrink_to_fit(); //release unused memory
        return path;
    }

    glm::vec3 prevPos = currentPos;
    currentPos = nextPos;

    //calculate the rest of the path
    for (int step = 1; step < maxSteps && totalLength < maxLength; step++)
    {
        nextPos = eulerIntegrate(currentPos, this->stepSize * direction);

        //check if the next point is still in bounds
        if (!inZeroMask(nextPos))
        {
            path.shrink_to_fit(); //release unused memory
            return path;
        }

        glm::vec3 prevDir = glm::normalize(currentPos - prevPos);
        glm::vec3 newDir = glm::normalize(nextPos - currentPos);

        float cosAngle = glm::dot(prevDir, newDir); //cos(a) = (u * v) / (|u| * |v|)
        //check if the angle between the vectors is too big
        //TODO something might be wrong with the angle constraint
        if (std::acosf(cosAngle) > this->maxAngle)
        {
            path.shrink_to_fit(); //release unused memory
            return path;
        }

        path.push_back(Point3D(nextPos.x, nextPos.y, nextPos.z));

        prevPos = currentPos;
        currentPos = nextPos;

        totalLength += this->stepSize; //discrete stepsize is used so no need to calculate the length of the actual vector
    }

    path.shrink_to_fit(); //release unused memory
    return path;
}

std::vector<std::vector<Point3D>> StreamlineTracer::traceAllStreamlines(const std::vector<Point3D>& seeds) {
    std::vector<std::vector<Point3D>> streamlines;
    streamlines.reserve(seeds.size());  // Pre-allocate memory

    // Use OpenMP for parallel processing
#pragma omp parallel
    {
        std::vector<std::vector<Point3D>> localStreamlines;

#pragma omp for nowait
        for (size_t i = 0; i < seeds.size(); i++) {
            std::vector<Point3D> streamline = traceStreamline(seeds[i]);

            // Only keep streamlines with sufficient points
            if (streamline.size() > 2) {
                localStreamlines.push_back(std::move(streamline));
            }
        }

        // Combine local results
#pragma omp critical
        {
            streamlines.insert(
                streamlines.end(),
                localStreamlines.begin(),
                localStreamlines.end()
            );
        }
    }

    return streamlines;
}