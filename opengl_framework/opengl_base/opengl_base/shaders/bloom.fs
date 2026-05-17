#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{
    vec3 sceneColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    
    // glow added over the final scene image, so you can see it through objects
    vec3 result = sceneColor + bloomColor;
    
    FragColor = vec4(result, 1.0);
}