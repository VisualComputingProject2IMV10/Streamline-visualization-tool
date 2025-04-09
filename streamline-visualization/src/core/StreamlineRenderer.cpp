#include "../include/StreamlineRenderer.h"
#include "../extra/glad.h"
#include <iostream>
#include <cmath>

StreamlineRenderer::StreamlineRenderer(Shader* shaderProgram, float width)
    : shader(shaderProgram), vertexCount(0), bufferIndexCount(0), lineWidth(width) {
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