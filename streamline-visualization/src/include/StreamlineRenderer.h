#pragma once

#include <vector>
#include "StreamlineTracer.h"
#include "Shader.h"

/**
 * @class StreamlineRenderer
 * @brief Handles rendering of streamlines in 3D space
 *
 * This class manages the OpenGL resources for visualizing streamlines generated
 * from a vector field. It provides methods for preparing streamlines for rendering,
 * filtering them based on spatial criteria, and rendering them with different
 * color schemes.
 */
class StreamlineRenderer {
public:
    /**
     * @brief Constructor
     * @param shaderProgram Shader to use for rendering
     * @param width Width of the streamlines in pixels
     */
    StreamlineRenderer(Shader* shaderProgram, float width = 1.0f);

    /**
     * @brief Destructor - cleans up OpenGL resources
     */
    ~StreamlineRenderer();

    /**
     * @brief Prepare a complete set of streamlines for rendering
     * @param streamlines Vector of streamlines to render
     * @param isToyDataset Flag for special coloring of toy dataset
     */
    void prepareStreamlines(const std::vector<std::vector<Point3D>>& streamlines, bool isToyDataset = false);

    /**
     * @brief Render the streamlines
     */
    void render() const;

    /**
     * @brief Set the line width for streamline rendering
     * @param width Width in pixels
     */
    void setLineWidth(float width) {
        lineWidth = width;
    }

private:
    unsigned int VAO;         ///< OpenGL Vertex Array Object
    unsigned int VBO;         ///< OpenGL Vertex Buffer Object
    unsigned int EBO;         ///< OpenGL element array buffer object
    Shader* shader;           ///< Shader program for rendering
    int vertexCount;          ///< Number of vertices in the streamlines
    int bufferIndexCount;      ///< number of indices of vertices to be drawn through the EBO
    float lineWidth;          ///< Width of streamlines in pixels
};