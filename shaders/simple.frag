#version 440 core

in vec2 uv;
in vec3 normal;

out vec4 FragColor;

void main() {
    FragColor = vec4(uv.x, uv.y, 0.0, 1.0);
}