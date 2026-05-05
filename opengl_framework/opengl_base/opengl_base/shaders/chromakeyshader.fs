#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D chromaKeyTexture;

void main() {
    vec4 rgbColor = texture(chromaKeyTexture, TexCoords);
    
    // RGB conversie to YCbCr
    float y  =  0.299 * rgbColor.r + 0.587 * rgbColor.g + 0.114 * rgbColor.b;
    float cb = -0.1687 * rgbColor.r - 0.3313 * rgbColor.g + 0.5 * rgbColor.b + 0.5;
    float cr =  0.5 * rgbColor.r - 0.4187 * rgbColor.g - 0.0813 * rgbColor.b + 0.5;
    
    // Green values
    vec2 targetGreen = vec2(0.35, 0.43); 
    
    // Calculate difference with current pixel
    float diff = distance(vec2(cb, cr), targetGreen);
    
    // If difference is small enough, then discard it
    if (diff < 0.15) {
        discard; 
    }

    FragColor = rgbColor;
}