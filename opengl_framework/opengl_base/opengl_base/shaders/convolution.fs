#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int effectType;

const float offset_x = 1.0 / 1280.0; // screen width
const float offset_y = 1.0 / 720.0;  // screen height

void main()
{
    float radius = 1.0;
    if (effectType == 1) {
        radius = 4.0; // make the blur stronger
    }

    vec2 offsets[9] = vec2[](
        vec2(-offset_x * radius,  offset_y * radius), // links-boven
        vec2( 0.0f,               offset_y * radius), // midden-boven
        vec2( offset_x * radius,  offset_y * radius), // rechts-boven
        vec2(-offset_x * radius,  0.0f),              // links-midden
        vec2( 0.0f,               0.0f),              // midden
        vec2( offset_x * radius,  0.0f),              // rechts-midden
        vec2(-offset_x * radius, -offset_y * radius), // links-onder
        vec2( 0.0f,              -offset_y * radius), // midden-onder
        vec2( offset_x * radius, -offset_y * radius)  // rechts-onder  
    );

    float kernel[9];

    if (effectType == 1) {
        kernel = float[](
            1.0 / 16, 2.0 / 16, 1.0 / 16,
            2.0 / 16, 4.0 / 16, 2.0 / 16,
            1.0 / 16, 2.0 / 16, 1.0 / 16  
        );
    } else if (effectType == 2) {
        kernel = float[](
            1,  1,  1,
            1, -8,  1,
            1,  1,  1
        );
    } else {
        kernel = float[](
            0, 0, 0,
            0, 1, 0,
            0, 0, 0
        );
    }

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }
    
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
    {
        col += sampleTex[i] * kernel[i];
    }
    
    FragColor = vec4(col, 1.0);
}