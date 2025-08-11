#version 440 core

// attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

// uniforms
uniform mat4 uMatrix;

// out to fragment shader
out vec2 uv;
out vec3 normal;

void main() {
    uv = aUV;
    normal = aNormal;
    vec4 pos = vec4(aPos, 1.0);
    pos = uMatrix * pos;
    gl_Position = pos; 
}