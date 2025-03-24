// anatomicalFragment.fs
#version 330 core

// Input from vertex shader
in vec3 vertColor;
in vec3 texCoord;

// Output color
out vec4 FragColor;

// Texture samplers
uniform sampler3D volumeTexture;   // Anatomical data
uniform sampler3D faTexture;       // Fractional anisotropy data

// Rendering parameters
uniform float alpha = 0.9;         // Opacity
uniform bool useDTIMode = false;   // Full DTI mode

void main()
{
    // Sample intensity from anatomical texture
    float intensity = texture(volumeTexture, texCoord).r;

    vec3 color;
    float outputAlpha;

    if (useDTIMode) {
        // DTI visualization mode - integrate fractional anisotropy

        // Sample from FA texture
        float fa = texture(faTexture, texCoord).r;

        // Create grayscale anatomical background with high contrast
        color = vec3(intensity * 1.5);

        // Apply FA-based transparency - higher FA values are more opaque
        outputAlpha = intensity * alpha * (fa * 0.5 + 0.5);

        // Enhance contrast by making dark areas fully transparent
        if (intensity < 0.1) {
            outputAlpha = 0.0;
        }
    } else {
        // Standard anatomical view

        // Enhance contrast for better visualization
        color = vec3(intensity * 1.2);

        // Make background darker
        if (intensity < 0.15) {
            color = vec3(0.05);
        }

        // Apply threshold-based transparency
        outputAlpha = intensity < 0.05 ? 0.0 : alpha;
    }

    // Output final color with alpha
    FragColor = vec4(color, outputAlpha);
}