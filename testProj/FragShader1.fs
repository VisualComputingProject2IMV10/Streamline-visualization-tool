#version 330
out vec4 FragColor;

in vec3 ourColor;
in vec3 texCoord;

uniform sampler3D ourTexture;

void main()
{
    //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    FragColor.rgb = texture(ourTexture, texCoord).rrr;
    //FragColor = vec4(ourColor, 1.0f);
} 