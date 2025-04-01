#pragma once

#include <vector>
#include <random>
#include "VectorField.h"

/**
 * @struct Point3D
 * @brief Simple 3D point structure for streamline representation
 */
struct Point3D {
    float x, y, z;

    Point3D() : x(0), y(0), z(0) {}
    Point3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

/**
 * @class StreamlineTracer
 * @brief Generates streamlines from vector fields
 *
 * This class handles the generation of streamlines from a 3D vector field,
 * including seed point generation, numerical integration, and termination criteria.
 */
class StreamlineTracer {
public:
    /**
     * @brief Constructor with streamline generation parameters
     * 
     * @param field Vector field to trace streamlines through
     * @param step Step size for numerical integration
     * @param steps Maximum number of steps per streamline
     * @param minMag Minimum vector magnitude before termination
     * @param maxLen Maximum length of a streamline
     */
    StreamlineTracer(VectorField* field, float step = 0.5f,
                    int steps = 2000, float minMag = 0.01f,
                    float maxLen = 100.0f, float maxAngle = 0.01f);

    /**
     * @brief Trace a single streamline from a seed point
     * 
     * @param seed Starting point for the streamline
     * 
     * @return Vector of points representing the streamline
     */
    std::vector<Point3D> traceStreamline(const Point3D& seed);

    /**
     * @brief Generate a uniform grid of seed points
     * 
     * @param strideX Spacing between seeds in X direction
     * @param strideY Spacing between seeds in Y direction
     * @param strideZ Spacing between seeds in Z direction
     * 
     * @return Vector of seed points
     */
    std::vector<Point3D> generateSeedGrid(int strideX = 5, int strideY = 5, int strideZ = 5);

    /**
     * @brief Generate seed points focused on brain anatomy
     * 
     * @param seedDensity Relative density of seed points
     * @param minIntensity Minimum intensity threshold for seed placement
     * 
     * @return Vector of seed points
     */
    std::vector<Point3D> generateUnifiedBrainSeeds(int seedDensity, float minIntensity = 0.15f);

    /**
     * @brief Generate seed points optimized for toy dataset
     * 
     * @param seedDensity Relative density of seed points
     * 
     * @return Vector of seed points
     */
    std::vector<Point3D> generateToyDatasetSeeds(int seedDensity);

    /**
     * @brief Trace streamlines from all provided seed points
     * @param seeds Vector of seed points
     * @return Vector of streamlines (each a vector of points)
     */
    std::vector<std::vector<Point3D>> traceAllStreamlines(const std::vector<Point3D>& seeds);

    std::vector<Point3D> generateSliceGridSeeds(int seedDensity, int slice);
    std::vector<std::vector<Point3D>> StreamlineTracer::traceVectors(std::vector<Point3D> seeds);

private:
    VectorField* vectorField;  ///< Reference to the vector field
    float stepSize;            ///< Step size for numerical integration
    int maxSteps;              ///< Maximum number of steps per streamline
    float minMagnitude;        ///< Minimum vector magnitude before termination
    float maxLength;           ///< Maximum length of a streamline
    float maxAngle;            ///< Max angle between vectors in the integration step

    /**
     * @brief Trace a streamline in one direction from a seed point
     * @param seed Starting point
     * @param direction Direction multiplier (1 or -1)
     * @return Vector of points representing the directional streamline
     */
    std::vector<Point3D> traceStreamlineDirection(const Point3D& seed, int direction);

    /**
     * @brief Perform Euler integration step
     * @param pos Current position
     * @param step Step size (can be negative for backward tracing)
     * @return Next position
     */
    Point3D eulerIntegrate(const Point3D& pos, float step);

    /**
     * @brief Perform 4th-order Runge-Kutta integration step
     * @param pos Current position
     * @param step Step size (can be negative for backward tracing)
     * @return Next position
     */
    Point3D rk4Integrate(const Point3D& pos, float step);

    //calculates v - u
    Point3D vecDiff(Point3D v, Point3D u);

    //normalizes a vector v
    Point3D normalize(Point3D v);

    void normalizeInPlace(Point3D v);

    float dot(Point3D v, Point3D u);
};