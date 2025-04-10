#include "../include/StreamlineTracer.h"
#include <cmath>
#include <iostream>
#include <random>

// External function declaration for scalar data sampling (implemented in Source.cpp)
extern float sampleScalarData(float x, float y, float z);

StreamlineTracer::StreamlineTracer(VectorField* field, float step, int steps, float maxLen, float maxAngle, const char* integrationMethod)
    : vectorField(field), stepSize(step), maxSteps(steps),
      maxLength(maxLen), maxAngle(maxAngle), integrationMethod(integrationMethod) {
    // Validate input parameters
    if (!field) {
        std::cerr << "Error: Null vector field provided to StreamlineTracer" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->zeroMask = field->getZeroMask(field->dimX, field->dimY, field->dimZ); //for quicker access
}

std::vector<Point3D> StreamlineTracer::generateSliceGridSeeds(int currentSliceX, int currentSliceY, int currentSliceZ, int axis)
{
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

    int dimX = vectorField->dimX;
    int dimY = vectorField->dimY;
    int dimZ = vectorField->dimZ;

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

    int dimX = vectorField->dimX;
    int dimY = vectorField->dimY;
    int dimZ = vectorField->dimZ;

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
    //calculate the volume of the seeding area
    float seedAttenuation = 0.5f;
    int maxSeeds = std::roundf(seedAttenuation * (1.3333f * 3.1415f * seedRadius * seedRadius * seedRadius) * density);
    seeds.reserve(maxSeeds); //pre allocate memory
    


    for (size_t i = 0; i < maxSeeds; i++)
    {
        //spherical random sampling
        float r = seedRadius * std::sqrtf((float) std::rand() / RAND_MAX);
        float theta = (float) std::rand() / RAND_MAX * 2 * std::_Pi_val;
        float phi = (float)std::rand() / RAND_MAX * std::_Pi_val;

        glm::vec3 seedPoint = glm::vec3(r * std::cosf(theta) * std::sinf(phi), r * std::sinf(theta) * std::sinf(phi), r * std::cosf(phi));
        
        if (axis == AXIS_X)
        {
            seedPoint += glm::vec3(sliceX, seedLoc.y, seedLoc.z);
        }
        else if (axis == AXIS_Y)
        {
            seedPoint += glm::vec3(seedLoc.x, sliceY, seedLoc.z);
        }
        else if (axis == AXIS_Z)
        {
            seedPoint += glm::vec3(seedLoc.x, seedLoc.y, sliceZ);
        }

        //check if the seed is valid
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

        Point3D head = Point3D(seed.x + vx, seed.y + vy, seed.z + vz);
        Point3D tail = Point3D(seed.x - vx, seed.y - vy, seed.z - vz);
        std::vector<Point3D> vec = { tail, seed, head };
        streamlines.push_back(vec);
    }

    std::cout << streamlines.size() << " streamlines computed" << std::endl;
    return streamlines;
}

std::vector<Point3D> StreamlineTracer::traceStreamline(const Point3D& seed) {
    std::vector<Point3D> streamline;

    // Validate seed point
    if (!vectorField->isInBounds(seed.x, seed.y, seed.z)) {
        return streamline;
    }

    // Trace in both directions from the seed point
    std::vector<Point3D> forwardPath = traceStreamlineDirection(seed, 1);
    std::vector<Point3D> backwardPath = traceStreamlineDirection(seed, -1);

    // Combine paths
    if (forwardPath.size() + backwardPath.size() > 0) //we skip empty paths
    {
        streamline.reserve(forwardPath.size() + backwardPath.size() + 1);

        streamline.insert(streamline.end(), backwardPath.rbegin(), backwardPath.rend());
        streamline.push_back(seed);
        streamline.insert(streamline.end(), forwardPath.begin(), forwardPath.end());
    }

    streamline.shrink_to_fit();

    return streamline;
}

bool StreamlineTracer::inZeroMask(glm::vec3 v)
{
    int x = std::roundf(v.x);
    int y = std::roundf(v.y);
    int z = std::roundf(v.z);
    
    //first sanity check if the coordinates are even in the domain of the mask
    if (x < 0 || x >= this->vectorField->dimX || y < 0 || y >= this->vectorField->dimY || z < 0 || z >= this->vectorField->dimZ)
    {
        return false;
    }

    //check the value of the zero mask at the point
    return this->zeroMask[x + y * this->vectorField->dimX + z * this->vectorField->dimX * this->vectorField->dimY];
}

glm::vec3 StreamlineTracer::rk2Integrate(glm::vec3 pos, float step)
{
    //x0 = pos
    glm::vec3 vx0; //v(x0)
    vectorField->interpolateVector(pos.x, pos.y, pos.z, vx0.x, vx0.y, vx0.z);

    //check for zero vector
    if (vx0 == glm::vec3(0.0f))
    {
        return pos;
    }

    glm::vec3 x1 = pos + 0.5f * step * glm::normalize(vx0);//midpoint

    glm::vec3 vx1;
    vectorField->interpolateVector(x1.x, x1.y, x1.z, vx1.x, vx1.y, vx1.z); //get the vector at the midpoint

    if (vx1 == glm::vec3(0.0f))
    {
        return pos;
    }

    glm::vec3 next = x1 + 0.5f * step * glm::normalize(vx1);
    return next;
}

glm::vec3 StreamlineTracer::eulerIntegrate(glm::vec3 pos, float step)
{
    //do first step manually so the loop can check angles more easily
    glm::vec3 vectorAtPos;
    vectorField->interpolateVector(pos.x, pos.y, pos.z, vectorAtPos.x, vectorAtPos.y, vectorAtPos.z);

    if (vectorAtPos == glm::vec3(0.0f))
    {
        return pos;
    }

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

    glm::vec3 nextPos;
    if (strcmp(this->integrationMethod, StreamlineTracer::EULER) == 0) //c string comparison
    {
        nextPos = eulerIntegrate(currentPos, this->stepSize * direction);
    }
    else if (strcmp(this->integrationMethod, StreamlineTracer::RUNGE_KUTTA_2ND_ORDER) == 0)
    {
        nextPos = rk2Integrate(currentPos, this->stepSize * direction);
    }
    else
    {
        std::cerr << "ERROR: invalid integration method given." << std::endl;
        path.shrink_to_fit();
        return path;
    }

    //check if the seed and next position are valid
    if (inZeroMask(nextPos) && inZeroMask(currentPos))
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
        glm::vec3 nextPos;
        if (strcmp(this->integrationMethod, StreamlineTracer::EULER) == 0)
        {
            nextPos = eulerIntegrate(currentPos, this->stepSize * direction);
        }
        else if (strcmp(this->integrationMethod, StreamlineTracer::RUNGE_KUTTA_2ND_ORDER) == 0)
        {
            nextPos = rk2Integrate(currentPos, this->stepSize * direction);
        }
        else
        {
            std::cerr << "ERROR: invalid integration method given." << std::endl;
            path.shrink_to_fit();
            return path;
        }

        //check if the algorithm hasn't hit a zero direction vector point since then it will get stuck
        if (nextPos == currentPos)
        {
            path.shrink_to_fit(); //release unused memory
            return path;
        }

        //check if the next point is still in bounds
        if (!inZeroMask(nextPos))
        {
            path.shrink_to_fit(); //release unused memory
            return path;
        }

        //printf("seed Vector: (%.2f, %.2f, %.2f)\n", prevPos.x, prevPos.y, prevPos.z);
        //printf("current Vector: (%.2f, %.2f, %.2f) direction: %i\n", currentPos.x, currentPos.y, currentPos.z, direction);
        //printf("next Vector: (%.2f, %.2f, %.2f)\n\n", nextPos.x, nextPos.y, nextPos.z);

        glm::vec3 prevDir = glm::normalize(currentPos - prevPos);
        glm::vec3 newDir = glm::normalize(nextPos - currentPos);

        //printf("previous vector: (%.2f, %.2f, %.2f)\n", prevDir.x, prevDir.y, prevDir.z);
        //printf("new vector: (%.2f, %.2f, %.2f)\n", newDir.x, newDir.y, newDir.z);

        float cosAngle = glm::dot(prevDir, newDir); //cos(a) = (u * v) / (|u| * |v|)
        //std::cout << "Cosangle: " << cosAngle << std::endl;
        //check if the angle between the vectors is too big
        //TODO something might be wrong with the angle constraint
        //printf("Vector: (%.2f, %.2f, %.2f) direction: %i\n", nextPos.x, nextPos.y, nextPos.z, direction);
        //std::cout << "Angle between vectors: " << std::acosf(cosAngle) << " max angle: " << this->maxAngle << std::endl;
        if (!(std::acosf(cosAngle) < this->maxAngle))
        {
            path.shrink_to_fit(); //release unused memory
            return path;
        }

        //std::cout << "direction: " << direction << std::endl;
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

    streamlines.shrink_to_fit();
    return streamlines;
}