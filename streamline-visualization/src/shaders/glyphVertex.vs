#version 330 core

// Input vertex attributes
layout (location = 0) in vec3 aPos;   // Vertex position
layout (location = 1) in vec3 aColor; // Vertex color

// Output values to fragment shader
out vec3 vertColor;

// Transformation matrices
uniform mat4 model;      // Model matrix (object to world space)
uniform mat4 view;       // View matrix (world to camera space)
uniform mat4 projection; // Projection matrix (camera to clip space)

void main()
{
    // Apply transformations to calculate clip space position
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Pass color to fragment shader
    vertColor = aColor;
}