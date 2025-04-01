#include "../include/StreamlineTracer.h"
#include <cmath>
#include <iostream>
#include <random>

#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>
#include <glm/geometric.hpp>

// External function declaration for scalar data sampling (implemented in Source.cpp)
extern float sampleScalarData(float x, float y, float z);

StreamlineTracer::StreamlineTracer(VectorField* field, float step, int steps,
                                 float minMag, float maxLen, float maxAngle)
    : vectorField(field), stepSize(step), maxSteps(steps),
      minMagnitude(minMag), maxLength(maxLen), maxAngle(maxAngle) {
    // Validate input parameters
    if (!field) {
        std::cerr << "Error: Null vector field provided to StreamlineTracer" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->zeroMask = field->getZeroMask(field->getDimX(), field->getDimY(), field->getDimZ()); //for quicker access
}

std::vector<Point3D> StreamlineTracer::generateSliceGridSeeds(int seedDensity, int slice)
{
    std::vector<Point3D> seeds;

    if (!vectorField) {
        std::cerr << "Error: Vector field is nullptr in generateUnifiedBrainSeeds" << std::endl;
        return seeds;
    }

    int dimX = vectorField->getDimX();
    int dimY = vectorField->getDimY();
    int dimZ = vectorField->getDimZ();

    if (slice < 0 || slice >= dimZ)
    {
        std::cerr << "Error: slice out of bounds" << std::endl;
        return seeds;
    }

    std::cout << "Generating simple grid slice seeds" << std::endl;

    //preallocate max memory needed for seeds
    seeds.reserve(dimX * dimY);

    for (int x = 0; x < dimX; x++)
    {
        for (int y = 0; y < dimY; y++)
        {
            if (this->zeroMask[x + y * dimX + slice * dimX * dimZ])
            {
                seeds.push_back(Point3D(x, y, slice));
            }
        }
    }
    seeds.shrink_to_fit();//free the unused memory

    std::cout << "Sampled " << seeds.size() << " seed points" << std::endl;
    return seeds;
}

std::vector<std::vector<Point3D>> StreamlineTracer::traceVectors(std::vector<Point3D> seeds)
{
    std::vector<std::vector<Point3D>> streamlines;
    streamlines.reserve(seeds.size());  // Pre-allocate memory

    for each(Point3D seed in seeds)
    {
        // Validate seed point
        if (!vectorField->isInBounds(seed.x, seed.y, seed.z)) {
            //std::cerr << "Seed point out of bounds: ("
           //     << seed.x << ", " << seed.y << ", " << seed.z << ")" << std::endl;
            continue;
        }

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

std::vector<Point3D> StreamlineTracer::generateUnifiedBrainSeeds(int seedDensity, float minIntensity) {
    std::vector<Point3D> seeds;

    if (!vectorField) {
        std::cerr << "Error: Vector field is nullptr in generateUnifiedBrainSeeds" << std::endl;
        return seeds;
    }

    int dimX = vectorField->getDimX();
    int dimY = vectorField->getDimY();
    int dimZ = vectorField->getDimZ();

    // Use a lower intensity threshold to capture more of the brain structure
    float adjustedIntensity = 0.08f;

    // Use higher seed density multiplier for better visualization
    int seedMultiplier = seedDensity * 1000;

    std::cout << "Generating enhanced unified brain seeds with density " << seedDensity << std::endl;

    // First create a map of all viable seed points
    std::vector<Point3D> viablePoints;

    // Scan the entire volume at a high resolution
    int stride = std::max(1, static_cast<int>(2.0f / seedDensity));

    std::cout << "Using stride: " << stride << " to find viable seed points" << std::endl;

    // First pass: find all viable seed points based on intensity and vector magnitude
    for (int x = 0; x < dimX; x += stride) {
        for (int y = 0; y < dimY; y += stride) {
            for (int z = 0; z < dimZ; z += stride) {
                float intensity = sampleScalarData(x, y, z);

                if (intensity > adjustedIntensity) {
                    float vx, vy, vz;
                    vectorField->interpolateVector(x, y, z, vx, vy, vz);

                    float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
                    if (magnitude > minMagnitude * 0.5f) {  // Lower threshold for inclusion
                        viablePoints.push_back(Point3D(x, y, z));
                    }
                }
            }
        }
    }

    std::cout << "Found " << viablePoints.size() << " viable seed points" << std::endl;

    // If no viable points found, try again with much lower threshold
    if (viablePoints.size() < 10) {
        adjustedIntensity = 0.01f;
        for (int x = 0; x < dimX; x += stride) {
            for (int y = 0; y < dimY; y += stride) {
                for (int z = 0; z < dimZ; z += stride) {
                    float intensity = sampleScalarData(x, y, z);

                    if (intensity > adjustedIntensity) {
                        float vx, vy, vz;
                        vectorField->interpolateVector(x, y, z, vx, vy, vz);

                        float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
                        if (magnitude > minMagnitude * 0.3f) {
                            viablePoints.push_back(Point3D(x, y, z));
                        }
                    }
                }
            }
        }
        std::cout << "Retry with lower threshold found " << viablePoints.size() << " viable points" << std::endl;
    }

    // Select seed points from viable points
    if (!viablePoints.empty()) {
        // Use high-quality random number generator
        std::random_device rd;
        std::mt19937 gen(rd());

        // Determine number of seeds based on density
        int numSeeds = std::min(static_cast<int>(viablePoints.size()), seedMultiplier);

        if (numSeeds == viablePoints.size()) {
            // Use all viable points
            seeds = viablePoints;
        } else {
            // Randomly sample points
            std::shuffle(viablePoints.begin(), viablePoints.end(), gen);
            seeds.assign(viablePoints.begin(), viablePoints.begin() + numSeeds);
        }

        // Add jittered duplicates of high-intensity regions for more detail
        std::vector<Point3D> extraSeeds;
        std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);

        for (const auto& seed : seeds) {
            // Sample intensity at this point
            float intensity = sampleScalarData(seed.x, seed.y, seed.z);

            // Add extra seeds around high-intensity points
            if (intensity > 0.25f) {
                for (int i = 0; i < 3; i++) {
                    float jx = seed.x + jitter(gen);
                    float jy = seed.y + jitter(gen);
                    float jz = seed.z + jitter(gen);

                    // Ensure in bounds
                    jx = std::max(0.0f, std::min(jx, static_cast<float>(dimX - 1)));
                    jy = std::max(0.0f, std::min(jy, static_cast<float>(dimY - 1)));
                    jz = std::max(0.0f, std::min(jz, static_cast<float>(dimZ - 1)));

                    extraSeeds.push_back(Point3D(jx, jy, jz));
                }
            }
        }

        // Add the extra seeds
        seeds.insert(seeds.end(), extraSeeds.begin(), extraSeeds.end());
    }

    std::cout << "Generated " << seeds.size() << " enhanced unified brain seed points" << std::endl;
    return seeds;
}

std::vector<Point3D> StreamlineTracer::generateToyDatasetSeeds(int seedDensity) {
    std::vector<Point3D> seeds;

    int dimX = vectorField->getDimX();
    int dimY = vectorField->getDimY();
    int dimZ = vectorField->getDimZ();

    // Toy datasets need denser seeding along the actual shapes
    // Use a fine-grained scan with a low intensity threshold
    float intensityThreshold = 0.05f;
    int stride = 1;  // Check every voxel

    std::cout << "Generating detailed toy dataset seeds" << std::endl;

    // First identify the shape's central voxels
    std::vector<Point3D> shapeVoxels;
    for (int x = 0; x < dimX; x++) {
        for (int y = 0; y < dimY; y++) {
            for (int z = 0; z < dimZ; z++) {
                float intensity = sampleScalarData(x, y, z);
                if (intensity > intensityThreshold) {
                    shapeVoxels.push_back(Point3D(x, y, z));
                }
            }
        }
    }

    std::cout << "Found " << shapeVoxels.size() << " shape voxels" << std::endl;

    // If we didn't find many voxels, try with an even lower threshold
    if (shapeVoxels.size() < 1000) {
        shapeVoxels.clear();
        intensityThreshold = 0.01f;

        for (int x = 0; x < dimX; x++) {
            for (int y = 0; y < dimY; y++) {
                for (int z = 0; z < dimZ; z++) {
                    float intensity = sampleScalarData(x, y, z);
                    if (intensity > intensityThreshold) {
                        shapeVoxels.push_back(Point3D(x, y, z));
                    }
                }
            }
        }

        std::cout << "Retry with lower threshold found " << shapeVoxels.size() << " shape voxels" << std::endl;
    }

    // Create seeds using shape voxels with meaningful vector directions
    for (const auto& voxel : shapeVoxels) {
        float vx, vy, vz;
        vectorField->getVector(voxel.x, voxel.y, voxel.z, vx, vy, vz);

        // Only add seeds where there's a meaningful vector direction
        float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
        if (magnitude > minMagnitude * 0.1f) {  // Use lower threshold
            seeds.push_back(voxel);
        }
    }

    // Apply additional sampling for TOY dataset if we have too many seeds
    if (seeds.size() > 10000) {
        // Randomly sample seeds
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(seeds.begin(), seeds.end(), g);

        // Limit to a reasonable number based on seed density
        int maxSeeds = seedDensity * 1000;
        if (seeds.size() > maxSeeds) {
            seeds.resize(maxSeeds);
        }
    }

    std::cout << "Generated " << seeds.size() << " toy dataset seed points" << std::endl;
    return seeds;
}

Point3D StreamlineTracer::eulerIntegrate(const Point3D& pos, float step) {
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
    streamline.clear();
    streamline.insert(streamline.end(), backwardPath.rbegin(), backwardPath.rend());
    streamline.push_back(seed);
    streamline.insert(streamline.end(), forwardPath.begin(), forwardPath.end());

    return streamline;
}

Point3D StreamlineTracer::vecDiff(Point3D v, Point3D u)
{
    Point3D result;
    result.x = v.x - u.x;
    result.y = v.y - u.y;
    result.z = v.z - u.z;
    return result;
}

Point3D StreamlineTracer::normalize(Point3D v)
{
    Point3D result;
    float length = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0)
    {
        result.x = v.x / length;
        result.y = v.y / length;
        result.z = v.z / length;
    }
    return result;
}

void StreamlineTracer::normalizeInPlace(Point3D v)
{
    float length = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0)
    {
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }
}

float StreamlineTracer::dot(Point3D v, Point3D u)
{
    return v.x * u.x + v.y * u.y + v.z * u.z;
}

std::vector<Point3D> StreamlineTracer::traceStreamlineDirection(const Point3D& seed, int direction) {
    std::vector<Point3D> path;
    Point3D currentPos = seed;
    float totalLength = 0.0f;

    for (int step = 0; step < maxSteps && totalLength < maxLength; step++) {
        // Get interpolated vector at current position
        float vx, vy, vz;
        vectorField->interpolateVector(currentPos.x, currentPos.y, currentPos.z, vx, vy, vz);

        // Check vector magnitude for termination
        float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
        if (magnitude < minMagnitude) break;

        // Use RK4 integration for better accuracy
        Point3D nextPos = rk4Integrate(currentPos, direction * stepSize);

        // Strict bounds checking
        if (!vectorField->isInBounds(nextPos.x, nextPos.y, nextPos.z)) break;

        //check if the angle is not too much
        //Point3D prevPos = path[path.size()];
        Point3D prevPos;
        if (!path.empty()) prevPos = path.back();

        //std::cout << "Prevpos: " << prevPos.x << " " << prevPos.y << " " << prevPos.z << std::endl;

        Point3D prevVec = normalize(vecDiff(currentPos, prevPos));
        Point3D nextVec = normalize(vecDiff(nextPos, currentPos));//TODO this is probably inefficient because we already have the vector somewhere
        
        if (dot(prevVec, nextVec) > std::cosf(maxAngle)) //TODO can optimize this away by putting the acos somewhere else
        {
            printf("greater than angle\n");
            break;
        }

        // Calculate step distance
        float stepDist = std::sqrt(
            (nextPos.x - currentPos.x) * (nextPos.x - currentPos.x) +
            (nextPos.y - currentPos.y) * (nextPos.y - currentPos.y) +
            (nextPos.z - currentPos.z) * (nextPos.z - currentPos.z)
        );

        // Add to path and update tracking
        path.push_back(nextPos);
        currentPos = nextPos;
        totalLength += stepDist;

        // Prevent infinite loops with extreme step count
        if (path.size() > maxSteps) break;
    }

    return path;
}

std::vector<Point3D> StreamlineTracer::generateSeedGrid(int densityX, int densityY, int densityZ) {
    std::vector<Point3D> seeds;
    seeds.reserve(densityX * densityY * densityZ);  // Pre-allocate memory

    // Use vector field dimensions
    int fieldDimX = vectorField->getDimX();
    int fieldDimY = vectorField->getDimY();
    int fieldDimZ = vectorField->getDimZ();

    // Use more efficient striding
    int strideX = std::max(1, fieldDimX / (densityX * 2));
    int strideY = std::max(1, fieldDimY / (densityY * 2));
    int strideZ = std::max(1, fieldDimZ / (densityZ * 2));

    for (int x = strideX; x < fieldDimX; x += strideX) {
        for (int y = strideY; y < fieldDimY; y += strideY) {
            for (int z = strideZ; z < fieldDimZ; z += strideZ) {
                // Quick magnitude check instead of full vector interpolation
                float vx, vy, vz;
                vectorField->getVector(x, y, z, vx, vy, vz);

                float magnitude = std::sqrt(vx*vx + vy*vy + vz*vz);
                if (magnitude > minMagnitude) {
                    seeds.emplace_back(x, y, z);
                }
            }
        }
    }

    return seeds;
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