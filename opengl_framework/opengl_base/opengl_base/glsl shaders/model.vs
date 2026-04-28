#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Bezier Deformation Uniforms
uniform bool applyDeformation;
uniform float modelLength;
uniform vec3 p0;
uniform vec3 p1;
uniform vec3 p2;
uniform vec3 p3;

vec3 evaluateBezier(float t, vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
    float u = 1.0 - t;
    return (u*u*u)*p0 + 3.0*(u*u)*t*p1 + 3.0*u*(t*t)*p2 + (t*t*t)*p3;
}

vec3 evaluateTangent(float t, vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
    float u = 1.0 - t;
    return 3.0*(u*u)*(p1 - p0) + 6.0*u*t*(p2 - p1) + 3.0*(t*t)*(p3 - p2);
}

void main()
{
    if (applyDeformation) {
        // Map the vertex to the curve's 't' parameter
        float t = (aPos.x / modelLength) + 0.5; 
        t = clamp(t, 0.0, 1.0);

        // Evaluate the curve
        vec3 curvePos = evaluateBezier(t, p0, p1, p2, p3);
        vec3 tangent = normalize(evaluateTangent(t, p0, p1, p2, p3));
        
        // Build an orthogonal frame
        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 bitangent = normalize(cross(tangent, up)); // points sideways
        vec3 normalVec = normalize(cross(bitangent, tangent)); // points up
        
        // Extract scale from the model matrix to keep width/height correct
        float scaleY = length(vec3(model[1])); 
        float scaleZ = length(vec3(model[2])); 

        // Deform using the Y and Z thickness of your rock strip (scaled appropriately)
        vec3 deformedPos = curvePos + (aPos.z * scaleZ * bitangent) + (aPos.y * scaleY * normalVec);
        
        // IMPORTANT: curvePos is ALREADY in world space, do not multiply by model matrix again!
        vec4 worldPos = vec4(deformedPos, 1.0);
        FragPos = vec3(worldPos);
        
        Normal = mat3(transpose(inverse(model))) * normalVec; 
        TexCoords = aTexCoords;

        gl_Position = projection * view * worldPos;

    } else {
        // Standard pass-through for un-deformed objects like the Tie Fighter
        vec4 worldPos = model * vec4(aPos, 1.0);
        FragPos = vec3(worldPos);
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        TexCoords = aTexCoords;
        gl_Position = projection * view * worldPos;
    }
}