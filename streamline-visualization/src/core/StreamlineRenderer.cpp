#include "src/include/StreamlineRenderer.h"
#include <src/extra/glad.h>
#include <iostream>
#include <cmath>

StreamlineRenderer::StreamlineRenderer(Shader* shaderProgram, float width)
    : shader(shaderProgram), vertexCount(0), lineWidth(width), colorMode(GRADIENT_COLOR) {
    // Initialize OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

StreamlineRenderer::~StreamlineRenderer() {
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void StreamlineRenderer::prepareStreamlines(const std::vector<std::vector<Point3D>>& streamlines, bool isToyDataset) {
    // Format for each vertex: [x,y,z,r,g,b]
    std::vector<float> vertices;

    // Process each streamline
    for (const auto& streamline : streamlines) {
        // Skip empty streamlines
        if (streamline.empty()) continue;

        if (isToyDataset) {
            // Special coloring for toy datasets to match the PDF example
            for (size_t i = 0; i < streamline.size() - 1; i++) {
                // Normalize coordinates based on position in the dataset
                float x_norm = streamline[i].x / 100.0f; // Adjust denominator based on domain size
                float y_norm = streamline[i].y / 100.0f;
                float z_norm = streamline[i].z / 100.0f;

                // Use position to determine color (RGB)
                // This creates distinct color regions similar to the PDF
                float r = std::abs(sin(x_norm * 3.14159f));
                float g = std::abs(sin(y_norm * 3.14159f + 2.0f));
                float b = std::abs(sin(z_norm * 3.14159f + 4.0f));

                // Enhance colors for better visibility
                r = 0.2f + 0.8f * r;
                g = 0.2f + 0.8f * g;
                b = 0.2f + 0.8f * b;

                // Add both points of the line segment
                // First point of segment
                vertices.push_back(streamline[i].x);
                vertices.push_back(streamline[i].y);
                vertices.push_back(streamline[i].z);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);

                // Second point of segment
                vertices.push_back(streamline[i+1].x);
                vertices.push_back(streamline[i+1].y);
                vertices.push_back(streamline[i+1].z);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
            }
        } else {
            // Original coloring for brain datasets
            // Generate initial color based on streamline's starting position
            // This creates variation between different streamlines
            float r = 0.5f + 0.5f * sin(streamline[0].x * 0.1f);
            float g = 0.5f + 0.5f * sin(streamline[0].y * 0.1f + 2.0f);
            float b = 0.5f + 0.5f * sin(streamline[0].z * 0.1f + 4.0f);

            // Add vertices for line segments with appropriate coloring
            for (size_t i = 0; i < streamline.size() - 1; i++) {
                float t = static_cast<float>(i) / static_cast<float>(streamline.size() - 1);
                float next_t = static_cast<float>(i+1) / static_cast<float>(streamline.size() - 1);

                if (colorMode == DIRECTION_COLOR) {
                    // Direction-based coloring
                    float dirX = streamline[i+1].x - streamline[i].x;
                    float dirY = streamline[i+1].y - streamline[i].y;
                    float dirZ = streamline[i+1].z - streamline[i].z;

                    // Normalize the direction
                    float mag = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
                    if (mag > 0) {
                        dirX /= mag; dirY /= mag; dirZ /= mag;
                    }

                    // Direction-based color (RGB = XYZ direction)
                    float dirR = fabs(dirX);
                    float dirG = fabs(dirY);
                    float dirB = fabs(dirZ);

                    // First point
                    vertices.push_back(streamline[i].x);
                    vertices.push_back(streamline[i].y);
                    vertices.push_back(streamline[i].z);
                    vertices.push_back(dirR);
                    vertices.push_back(dirG);
                    vertices.push_back(dirB);

                    // Second point
                    vertices.push_back(streamline[i+1].x);
                    vertices.push_back(streamline[i+1].y);
                    vertices.push_back(streamline[i+1].z);
                    vertices.push_back(dirR);
                    vertices.push_back(dirG);
                    vertices.push_back(dirB);
                }
                else if (colorMode == MANUAL_SEED_COLOR) {
                    // Bright, high-contrast colors for manually seeded streamlines
                    // First point - bright color
                    vertices.push_back(streamline[i].x);
                    vertices.push_back(streamline[i].y);
                    vertices.push_back(streamline[i].z);
                    vertices.push_back(1.0f);  // Bright red
                    vertices.push_back(1.0f - t); // Yellow to red gradient
                    vertices.push_back(0.0f);

                    // Second point
                    vertices.push_back(streamline[i+1].x);
                    vertices.push_back(streamline[i+1].y);
                    vertices.push_back(streamline[i+1].z);
                    vertices.push_back(1.0f);
                    vertices.push_back(1.0f - next_t);
                    vertices.push_back(0.0f);
                }
                else {
                    // Original gradient coloring method
                    // First point of segment with its color
                    vertices.push_back(streamline[i].x);
                    vertices.push_back(streamline[i].y);
                    vertices.push_back(streamline[i].z);
                    vertices.push_back(r * (1.0f-t) + t * 1.0f); // Fade to red
                    vertices.push_back(g);
                    vertices.push_back(b * (1.0f-t));

                    // Second point of segment with its color
                    vertices.push_back(streamline[i+1].x);
                    vertices.push_back(streamline[i+1].y);
                    vertices.push_back(streamline[i+1].z);
                    vertices.push_back(r * (1.0f-next_t) + next_t * 1.0f);
                    vertices.push_back(g);
                    vertices.push_back(b * (1.0f-next_t));
                }
            }
        }
    }

    vertexCount = vertices.size() / 6; // 6 values per vertex (3 position, 3 color)

    // Bind the vertex array and buffer
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Upload vertex data
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Prepared " << streamlines.size() << " streamlines with "
              << vertexCount << " vertices" << std::endl;
}

void StreamlineRenderer::addStreamlines(const std::vector<std::vector<Point3D>>& newStreamlines) {
    if (newStreamlines.empty()) return;

    std::cout << "Adding " << newStreamlines.size() << " new streamlines" << std::endl;

    // Create vertices for the new streamlines
    std::vector<float> vertices;

    for (const auto& streamline : newStreamlines) {
        if (streamline.size() < 2) continue;

        // Generate color based on streamline start position
        float r = 0.5f + 0.5f * sin(streamline[0].x * 0.1f);
        float g = 0.5f + 0.5f * sin(streamline[0].y * 0.1f + 2.0f);
        float b = 0.5f + 0.5f * sin(streamline[0].z * 0.1f + 4.0f);

        // Add vertices for line segments
        for (size_t i = 0; i < streamline.size() - 1; i++) {
            float t = static_cast<float>(i) / static_cast<float>(streamline.size() - 1);
            float next_t = static_cast<float>(i+1) / static_cast<float>(streamline.size() - 1);

            // First point of segment
            vertices.push_back(streamline[i].x);
            vertices.push_back(streamline[i].y);
            vertices.push_back(streamline[i].z);
            vertices.push_back(r * (1.0f-t) + t);
            vertices.push_back(g);
            vertices.push_back(b * (1.0f-t));

            // Second point of segment
            vertices.push_back(streamline[i+1].x);
            vertices.push_back(streamline[i+1].y);
            vertices.push_back(streamline[i+1].z);
            vertices.push_back(r * (1.0f-next_t) + next_t);
            vertices.push_back(g);
            vertices.push_back(b * (1.0f-next_t));
        }
    }

    int newVertexCount = vertices.size() / 6;
    if (newVertexCount == 0) return;

    std::cout << "Adding " << newVertexCount << " new vertices to renderer" << std::endl;

    // Get existing vertices from current VBO
    std::vector<float> existingVertices;
    if (vertexCount > 0) {
        existingVertices.resize(vertexCount * 6);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, existingVertices.size() * sizeof(float), existingVertices.data());
    }

    // Combine existing and new vertices
    existingVertices.insert(existingVertices.end(), vertices.begin(), vertices.end());

    // Update the total vertex count
    vertexCount = existingVertices.size() / 6;

    // Update the VBO with all vertices
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, existingVertices.size() * sizeof(float), existingVertices.data(), GL_STATIC_DRAW);

    // Re-specify vertex attributes (in case VAO setup was lost)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "Now rendering " << vertexCount << " total vertices" << std::endl;
}

void StreamlineRenderer::filterStreamlinesForSlice(const std::vector<std::vector<Point3D>>& streamlines,
                                                 int sliceAxis, int slicePos, int threshold) {
    std::vector<float> vertices;

    std::cout << "Filtering streamlines near slice axis " << sliceAxis
              << " at position " << slicePos << " with threshold " << threshold << std::endl;

    int filteredCount = 0;
    for (const auto& streamline : streamlines) {
        if (streamline.empty()) continue;

        // Check if any point in the streamline is close to the slice
        bool isCloseToSlice = false;
        for (const auto& point : streamline) {
            float pointCoord;
            if (sliceAxis == 0) pointCoord = point.x;
            else if (sliceAxis == 1) pointCoord = point.y;
            else pointCoord = point.z;

            if (std::abs(pointCoord - slicePos) <= threshold) {
                isCloseToSlice = true;
                break;
            }
        }

        if (!isCloseToSlice) continue;
        filteredCount++;

        // Generate initial color based on streamline's starting position
        float r = 0.5f + 0.5f * sin(streamline[0].x * 0.1f);
        float g = 0.5f + 0.5f * sin(streamline[0].y * 0.1f + 2.0f);
        float b = 0.5f + 0.5f * sin(streamline[0].z * 0.1f + 4.0f);

        // Add vertices for line segments with color gradient
        for (size_t i = 0; i < streamline.size() - 1; i++) {
            float t = static_cast<float>(i) / static_cast<float>(streamline.size() - 1);
            float next_t = static_cast<float>(i+1) / static_cast<float>(streamline.size() - 1);

            // First point of segment with its color
            vertices.push_back(streamline[i].x);
            vertices.push_back(streamline[i].y);
            vertices.push_back(streamline[i].z);
            vertices.push_back(r * (1.0f-t) + t * 1.0f); // Fade to red
            vertices.push_back(g);
            vertices.push_back(b * (1.0f-t));

            // Second point of segment with its color
            vertices.push_back(streamline[i+1].x);
            vertices.push_back(streamline[i+1].y);
            vertices.push_back(streamline[i+1].z);
            vertices.push_back(r * (1.0f-next_t) + next_t * 1.0f);
            vertices.push_back(g);
            vertices.push_back(b * (1.0f-next_t));
        }
    }

    vertexCount = vertices.size() / 6; // 6 values per vertex (3 position, 3 color)

    std::cout << "Filtered from " << streamlines.size() << " to " << filteredCount
              << " streamlines with " << vertexCount << " vertices" << std::endl;

    // Bind the vertex array and buffer
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Upload vertex data
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void StreamlineRenderer::render() const {
    if (vertexCount == 0) return;

    shader->use();

    // Set line width
    glLineWidth(lineWidth);

    // Draw lines
    glBindVertexArray(VAO);

    // Enable depth test but make lines visible above slice
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDrawArrays(GL_LINES, 0, vertexCount);
    glBindVertexArray(0);
}