#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 lightColor;
uniform float intensity;

void main() {

    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 result = texColor.rgb * lightColor * intensity;
    FragColor = vec4(result, texColor.a);
}