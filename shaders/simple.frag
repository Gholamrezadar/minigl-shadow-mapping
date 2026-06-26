#version 440 core

in vec2 uv;
in vec3 normal;
in vec4 world_pos;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform vec3 uLightAmbient;
uniform float uLightSize;
uniform float uShininess;
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uBias;
uniform float uNormalBias;

// TODO: shininess uniform

// camera
uniform vec3 uCameraPos;

out vec4 FragColor;

vec3 toneMapReinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

// 0: Shadow, 1: No Shadow
float ShadowCalculation(vec4 fragWorldPos, vec3 N, vec3 L)
{
    // Apply normal bias scaled by grazing angle to reduce acne on steep surfaces
    // "Basically, during the generation of the shadow map, this will inset the geometry in"
    // - https://doc.babylonjs.com/features/featuresDeepDive/lights/shadows/
    float angleFactor = 1.0 - max(dot(N, L), 0.0);
    vec3 biasedWorldPos = fragWorldPos.xyz + N * (uNormalBias * angleFactor);

    // Transform biased position to light space
    vec4 fragLightSpacePos = uLightSpaceMatrix * vec4(biasedWorldPos, 1.0);
    vec3 proj = fragLightSpacePos.xyz / fragLightSpacePos.w;
    proj = proj * 0.5 + 0.5;

    float closestDepth = texture(uShadowMap, proj.xy).r;
    float currentDepth = proj.z;

    float shadow = currentDepth > closestDepth + uBias ? 0.0 : 1.0;
    return shadow;
}

void main() {
    vec3 N = normalize(normal);
    vec3 L = normalize(-uLightDirection);
    vec3 V = normalize(uCameraPos - world_pos.xyz);
    vec3 H = normalize(L + V);

    // ambient
    vec3 ambient = uLightAmbient;

    // diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = uLightColor * NdotL;

    // specular
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, uShininess);
    vec3 specular = uLightColor * spec * 0.25;

    // direct lighting
    vec3 direct = (diffuse + specular) * uLightIntensity;
    direct *= ShadowCalculation(world_pos, N, L);

    vec3 color = ambient + direct;

    // tone mapping
    color = toneMapReinhard(color);

    // gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}