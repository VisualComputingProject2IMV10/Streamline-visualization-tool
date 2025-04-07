#pragma once

#include <vector>
#include <random>
#include "VectorField.h"
#include "Constants.h"

#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>
#include <glm/geometric.hpp>

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
    StreamlineTracer(VectorField* field, float step = 0.5f, int steps = 2000, float maxLen = 100.0f, float maxAngle = 0.01f);

    /**
     * @brief Trace a single streamline from a seed point
     * 
     * @param seed Starting point for the streamline
     * 
     * @return Vector of points representing the streamline
     */
    std::vector<Point3D> traceStreamline(const Point3D& seed);

    /**
     * @brief Trace streamlines from all provided seed points
     * @param seeds Vector of seed points
     * @return Vector of streamlines (each a vector of points)
     */
    std::vector<std::vector<Point3D>> traceAllStreamlines(const std::vector<Point3D>& seeds);

    std::vector<Point3D> generateSliceGridSeeds(int currentSliceX, int currentSliceY, int currentSliceZ, int axis);

    std::vector<Point3D> generateMouseSeeds(int slice, glm::vec3 seedLoc, float seedRadius, float density);

    std::vector<std::vector<Point3D>> StreamlineTracer::traceVectors(std::vector<Point3D> seeds);

    float stepSize;            ///< Step size for numerical integration
    int maxSteps;              ///< Maximum number of steps per streamline
    float maxLength;           ///< Maximum length of a streamline
    float maxAngle;         ///< Cosine of the max angle between vectors in the integration step

private:
    VectorField* vectorField;  ///< Reference to the vector field
    bool* zeroMask;            ///< the zero mask of the vector field

    /**
     * Returns wether the rounded vector is in the zeromask.
     */
    bool inZeroMask(glm::vec3 v);

    /**
     * @brief Trace a streamline in one direction from a seed point
     * @param seed Starting point
     * @param direction Direction multiplier (1 or -1)
     * @return Vector of points representing the directional streamline
     */
    std::vector<Point3D> traceStreamlineDirection(const Point3D& seed, int direction);
    std::vector<Point3D> traceStreamlineDirection1(const Point3D& seed, int direction);

    /**
     * @brief Perform Euler integration step
     * @param pos Current position
     * @param step Step size (can be negative for backward tracing)
     * @return Next position
     */
    glm::vec3 eulerIntegrate(glm::vec3 pos, float step);
    Point3D eulerIntegrate1(const Point3D& pos, float step);

    /**
     * @brief Perform 4th-order Runge-Kutta integration step
     * @param pos Current position
     * @param step Step size (can be negative for backward tracing)
     * @return Next position
     */
    Point3D rk4Integrate(const Point3D& pos, float step);
};