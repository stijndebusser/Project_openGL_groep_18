#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int mode;

// mode 0 = normal
// mode 1 = blur
// mode 2 = edge detection

void main()
{
    if (mode == 0)
    {
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    float offset = 1.0 / 300.0;

    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset),
        vec2( 0.0,     offset),
        vec2( offset,  offset),
        vec2(-offset,  0.0),
        vec2( 0.0,     0.0),
        vec2( offset,  0.0),
        vec2(-offset, -offset),
        vec2( 0.0,    -offset),
        vec2( offset, -offset)
    );

    float blurKernel[9] = float[](
        1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
        2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
        1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
    );

    float edgeKernel[9] = float[](
         1.0,  1.0,  1.0,
         1.0, -8.0,  1.0,
         1.0,  1.0,  1.0
    );

    vec3 sampleTex[9];

    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords + offsets[i]));
    }

    vec3 color = vec3(0.0);

    for (int i = 0; i < 9; i++)
    {
        if (mode == 1)
        {
            color += sampleTex[i] * blurKernel[i];
        }
        else
        {
            color += sampleTex[i] * edgeKernel[i];
        }
    }

    FragColor = vec4(color, 1.0);
}