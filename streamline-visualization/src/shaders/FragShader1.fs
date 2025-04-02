#version 330 core

// Input from vertex shader
//in vec3 vertColor;
in vec3 texCoord;

// Output color
out vec4 FragColor;

// Texture samplers
uniform sampler3D volumeTexture;   // Anatomical data
uniform sampler3D faTexture;       // Optional FA (fractional anisotropy) data

uniform float currentSlice; //the texture coord of the current slice

// Rendering parameters
//uniform float alpha = 0.9;         // Opacity
//uniform int visualizationMode = 1; // Default visualization mode

void main()
{
    
    vec3 newTexCoord = vec3(texCoord.st, currentSlice);

    // Sample intensity from volumetric texture
    float intensity = texture(volumeTexture, newTexCoord).r;
    float alpha = texture(volumeTexture, newTexCoord).g; //alpha is encoded as green in the 3d texture
    FragColor = vec4(vec3(intensity), alpha);
    
    //FragColor = vec4(1.0f);



    /*
    vec3 color;
    float outputAlpha;

    // Apply different visualization modes
    switch(visualizationMode) {
        case 0: // Grayscale - simple direct mapping
            color = vec3(intensity);
            outputAlpha = intensity < 0.05 ? 0.0 : alpha;
            break;

        case 1: // Anatomical - enhanced contrast
            // Enhance contrast for anatomical view
            intensity = pow(intensity, 0.5);
            color = vec3(intensity * 1.2);

            // Make background darker
            if (intensity < 0.15) {
                color = vec3(0.05);
            }

            outputAlpha = intensity < 0.05 ? 0.0 : alpha;
            break;

        case 2: // White Matter - high contrast for white matter
            // Higher contrast for white matter visibility
            intensity = pow(intensity, 0.3);
            color = vec3(intensity * 1.5);
            outputAlpha = intensity < 0.1 ? 0.0 : alpha;
            break;

        case 3: // Directional - using FA texture
            // Use FA values to guide visualization
            float fa = texture(faTexture, texCoord).r;
            color = vec3(intensity * 1.5);
            outputAlpha = intensity * alpha * (fa * 0.5 + 0.5);

            // Enhance contrast by making dark areas fully transparent
            if (intensity < 0.1) {
                outputAlpha = 0.0;
            }
            break;

        default:
            // Fallback mode
            color = vec3(intensity);
            outputAlpha = alpha;
            break;
    }

    // Output final color with alpha
    FragColor = vec4(color, outputAlpha);
    */
}