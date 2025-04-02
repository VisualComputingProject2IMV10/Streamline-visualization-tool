#include "../include/StreamlineRenderer.h"
#include "../extra/glad.h"
#include <iostream>
#include <cmath>

StreamlineRenderer::StreamlineRenderer(Shader* shaderProgram, float width)
    : shader(shaderProgram), vertexCount(0), lineWidth(width), colorMode(GRADIENT_COLOR) {
    // Initialize OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

StreamlineRenderer::~StreamlineRenderer() {
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void StreamlineRenderer::prepareStreamlines(const std::vector<std::vector<Point3D>>& streamlines, bool isToyDataset) {
    // Format for each vertex: [x,y,z,r,g,b]
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int currentIndex = 0;

    //precalculate the needed space for the vectors so we don't have to reallocate memory every step
    int totalVertsSize = 0;
    int totalIndicesSize = 0;
    for each(std::vector<Point3D> streamline in streamlines)
    {
        totalIndicesSize += streamline.size() + 1; //+1 for the primitive restart
        totalVertsSize += streamline.size() * 6; //6 vertex attributes
    }
    vertices.resize(totalVertsSize);
    indices.reserve(totalIndicesSize);

    for (int i = 0; i < streamlines.size(); i++)
    {
        if (streamlines[i].empty()) continue;
        float r, g, b;
        for (int j = 0; j < streamlines[i].size()-1; j++)
        {
            //get color based on direction
            r = std::abs(streamlines[i][j + 1].x - streamlines[i][j].x);
            g = std::abs(streamlines[i][j + 1].y - streamlines[i][j].y);
            b = std::abs(streamlines[i][j + 1].z - streamlines[i][j].z);
            //normalize
            float l = std::sqrtf(r * r + g * g + b * b);
            r /= l;
            g /= l;
            b /= l;
            
            //Add point to segment
            vertices[currentIndex * 6 + 0] = streamlines[i][j].x;
            vertices[currentIndex * 6 + 1] = streamlines[i][j].y;
            vertices[currentIndex * 6 + 2] = streamlines[i][j].z;
            vertices[currentIndex * 6 + 3] = r;
            vertices[currentIndex * 6 + 4] = g;
            vertices[currentIndex * 6 + 5] = b;
            
            indices.push_back(currentIndex);
            currentIndex++;
        }

        //manually add last vertex since we can't calculate the angle
        vertices[currentIndex * 6 + 0] = streamlines[i][streamlines[i].size() - 1].x;
        vertices[currentIndex * 6 + 1] = streamlines[i][streamlines[i].size() - 1].y;
        vertices[currentIndex * 6 + 2] = streamlines[i][streamlines[i].size() - 1].z;
        vertices[currentIndex * 6 + 3] = r; //definitely ignore this compiler warning
        vertices[currentIndex * 6 + 4] = g;
        vertices[currentIndex * 6 + 5] = b;

        indices.push_back(currentIndex);
        currentIndex++;
        indices.push_back(0xFFFF);//primitive restart fixed index

    }

    vertexCount = vertices.size() / 6; // 6 values per vertex (3 position, 3 color)
    bufferIndexCount = indices.size();

    // Bind the vertex array and buffer
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Upload vertex data
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Unbind
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

    glDrawElements(GL_LINE_STRIP, bufferIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}