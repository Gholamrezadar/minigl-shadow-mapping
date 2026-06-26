#version 440 core

// attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

// uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// out to fragment shader
out vec2 uv;
out vec3 normal;
out vec4 world_pos;

void main() {
    uv = aUV;

    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    normal = normalMatrix * aNormal;

    vec4 pos = vec4(aPos, 1.0);
    mat4 PVM = uProjection * uView * uModel;
    world_pos = uModel * pos;
    gl_Position = PVM * pos; 
}