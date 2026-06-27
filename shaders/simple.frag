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
uniform bool uPCF; 
uniform float uPCFRadius;
uniform bool uPCFPoisson;

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

    // No PCF
    if(!uPCF) {
        float closestDepth = texture(uShadowMap, proj.xy).r;
        float currentDepth = proj.z;
        float shadow = currentDepth > closestDepth + uBias ? 0.0 : 1.0;
        return shadow;
    }
    if(uPCF && !uPCFPoisson) {
        float shadow = 0.0;
        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
        float currentDepth = proj.z;

        // 5 x 5 kernel
        for (int y = -2; y <= 2; ++y) {
            for (int x = -2; x <= 2; ++x) {
                vec2 offset = vec2(x, y) * texelSize * uPCFRadius;
                float depth = texture(uShadowMap, proj.xy + offset).r;

                shadow += (currentDepth > depth + uBias) ? 0.0 : 1.0;
            }
        }

        shadow /= 25.0;
        return shadow;
    }
    if (uPCF && uPCFPoisson) {
        const vec2 poisson[16] = vec2[](
            vec2(-0.94201624, -0.39906216),
            vec2( 0.94558609, -0.76890725),
            vec2(-0.09418410, -0.92938870),
            vec2( 0.34495938,  0.29387760),
            vec2(-0.91588581,  0.45771432),
            vec2(-0.81544232, -0.87912464),
            vec2(-0.38277543,  0.27676845),
            vec2( 0.97484398,  0.75648379),
            vec2( 0.44323325, -0.97511554),
            vec2( 0.53742981, -0.47373420),
            vec2(-0.26496911, -0.41893023),
            vec2( 0.79197514,  0.19090188),
            vec2(-0.24188840,  0.99706507),
            vec2(-0.81409955,  0.91437590),
            vec2( 0.19984126,  0.78641367),
            vec2( 0.14383161, -0.14100790)
        );
        // Random angle based on fragment position to break pattern repetition
        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        float angle = noise * 6.28318; // 2*PI
        float s = sin(angle);
        float c = cos(angle);
        mat2 rotation = mat2(c, -s, s, c);

        float shadow = 0.0;
        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
        float currentDepth = proj.z;

        for (int i = 0; i < 16; ++i) {
            // Rotate the sample offset per fragment
            vec2 offset = rotation * poisson[i] * texelSize * uPCFRadius;
            float depth = texture(uShadowMap, proj.xy + offset).r;
            shadow += (currentDepth > depth + uBias) ? 0.0 : 1.0;
        }

        shadow /= 16.0;
        return shadow;
    }
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