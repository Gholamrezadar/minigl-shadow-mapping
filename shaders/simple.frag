#version 440 core

in vec2 uv;
in vec3 normal;
in vec3 world_pos;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform vec3 uLightAmbient;
uniform float uLightSize;

// TODO: shininess uniform

// camera
uniform vec3 uCameraPos;

out vec4 FragColor;

vec3 toneMapReinhard(vec3 x) {
    return x / (x + vec3(1.0));
}


void main() {
    vec3 N = normalize(normal);
    vec3 L = normalize(-uLightDirection);
    vec3 V = normalize(uCameraPos - world_pos);
    vec3 H = normalize(L + V);

    // ambient
    vec3 ambient = uLightAmbient;

    // diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = uLightColor * NdotL;

    // specular
    float NdotH = max(dot(N, H), 0.0);
    float shininess = 32.0;
    float spec = pow(NdotH, shininess);
    vec3 specular = uLightColor * spec * 0.25;

    // direct lighting (multiply by shadow later)
    vec3 direct = (diffuse + specular) * uLightIntensity;

    vec3 color = ambient + direct;

    // tone mapping
    color = toneMapReinhard(color);

    // gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}