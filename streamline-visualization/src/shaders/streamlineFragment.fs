#version 330 core

// Input from vertex shader
in vec3 vertColor;

// Output color
out vec4 FragColor;

void main()
{
    // Output the vertex color with full opacity
    FragColor = vec4(vertColor, 1.0);
}