#version 330 core

// Input from vertex shader
//in vec3 vertColor;
in vec3 texCoord;

// Output color
out vec4 FragColor;

// Texture samplers
uniform sampler3D volumeTexture;   // Anatomical data

uniform float currentSlice; //the texture coord of the current slice
uniform int selectedAxis;

void main()
{
    vec3 newTexCoord;
    if (selectedAxis == 2)
    {
        newTexCoord = vec3(texCoord.st, currentSlice);
    } 
    else if (selectedAxis == 1)
    {
        newTexCoord = vec3(texCoord.s, currentSlice, texCoord.p);
    } 
    else //selectedAxis == 0
    {
        newTexCoord = vec3(currentSlice, texCoord.tp);
    }

    // Sample intensity from volumetric texture
    float intensity = texture(volumeTexture, newTexCoord).r;
    float alpha = texture(volumeTexture, newTexCoord).g; //alpha is encoded as green in the 3d texture
    FragColor = vec4(vec3(intensity), alpha);
}