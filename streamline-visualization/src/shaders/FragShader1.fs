#version 330 core

// Input from vertex shader
//in vec3 vertColor;
in vec3 texCoord;

// Output color
out vec4 FragColor;

// Texture samplers
uniform sampler3D volumeTexture;   // Anatomical data

uniform float currentSlice; //the texture coord of the current slice

void main()
{
    vec3 newTexCoord = vec3(texCoord.st, currentSlice);

    // Sample intensity from volumetric texture
    float intensity = texture(volumeTexture, newTexCoord).r;
    float alpha = texture(volumeTexture, newTexCoord).g; //alpha is encoded as green in the 3d texture
    FragColor = vec4(vec3(intensity), alpha);
}