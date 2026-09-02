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
uniform float uPCFRadius;

uniform int uShadowMode;

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

    // PCSS
    if (uShadowMode == 4) {
        const int POISSON_SAMPLES = 64;

        const vec2 poisson[64] = vec2[](
            vec2(-0.20247402,  0.15207597),
            vec2( 0.20149906, -0.86408825),
            vec2(-0.20054118,  0.72700785),
            vec2( 0.69409233,  0.12440118),
            vec2( 0.28097072, -0.29987406),
            vec2( 0.46928529, -0.45704720),
            vec2( 0.46028738,  0.24683710),
            vec2(-0.61937022, -0.19674099),
            vec2(-0.52993902,  0.42094412),
            vec2(-0.40316085, -0.77289486),
            vec2(-0.61481114, -0.66933754),
            vec2( 0.55550126, -0.68524955),
            vec2(-0.33689123, -0.50331671),
            vec2( 0.31626476, -0.62017842),
            vec2( 0.14490484,  0.46227538),
            vec2( 0.26529645,  0.66285755),
            vec2( 0.73385906,  0.51006072),
            vec2( 0.39544421,  0.49502017),
            vec2(-0.11624895, -0.26938865),
            vec2(-0.59811618,  0.67845403),
            vec2(-0.39357397, -0.05989137),
            vec2(-0.54517939, -0.42013838),
            vec2(-0.14568419,  0.46613697),
            vec2(-0.03260006, -0.89229408),
            vec2( 0.47928570, -0.19424873),
            vec2(-0.07627513,  0.91188786),
            vec2(-0.73246782,  0.52806083),
            vec2( 0.61436998,  0.39249953),
            vec2( 0.41038317, -0.81881637),
            vec2( 0.69935661, -0.55989159),
            vec2(-0.11781136, -0.48628885),
            vec2( 0.01548382,  0.67209550),
            vec2(-0.87231756,  0.32329777),
            vec2(-0.16840695, -0.07321553),
            vec2( 0.05567108,  0.05270869),
            vec2( 0.82874030, -0.32331492),
            vec2( 0.87016131, -0.11856777),
            vec2(-0.00132784,  0.28348242),
            vec2(-0.21698684, -0.88288154),
            vec2(-0.74772178, -0.52003833),
            vec2(-0.88747313, -0.09830558),
            vec2( 0.63532489, -0.33616059),
            vec2( 0.49452683,  0.02334738),
            vec2(-0.12092560, -0.68853546),
            vec2(-0.36246847,  0.82170153),
            vec2(-0.37417472,  0.60935246),
            vec2(-0.43501726,  0.18010042),
            vec2(-0.29625035,  0.36166212),
            vec2(-0.81693909, -0.29980647),
            vec2(-0.89268187,  0.11725455),
            vec2( 0.88906423,  0.07909920),
            vec2( 0.12665851, -0.41721299),
            vec2( 0.59386370,  0.66730540),
            vec2(-0.67195016,  0.26836286),
            vec2( 0.08532004, -0.17513422),
            vec2( 0.84824945,  0.30379112),
            vec2( 0.26598470,  0.27446100),
            vec2( 0.08171311, -0.66678970),
            vec2(-0.63940790,  0.04395555),
            vec2( 0.29414836,  0.01001298),
            vec2( 0.67396508, -0.10782049),
            vec2(-0.35232544, -0.27765891),
            vec2( 0.38869693,  0.83980514),
            vec2( 0.16636611,  0.88231662)
        );

        // Per-fragment rotation to break pattern repetition
        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        float angle = noise * 6.28318;
        float s = sin(angle);
        float c = cos(angle);
        mat2 rot = mat2(c, -s, s, c);

        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
        float currentDepth = proj.z;

        // Blocker search: average depth of shadow-map samples closer than the fragment
        // Search radius scales with light size and receiver distance from near plane
        float searchRadius = uLightSize * (currentDepth - 0.1) / currentDepth;
        float blockerSum = 0.0;
        int blockerCount = 0;

        for (int i = 0; i < POISSON_SAMPLES; ++i) {
            vec2 offset = rot * poisson[i] * texelSize * searchRadius;
            float blockerDepth = texture(uShadowMap, proj.xy + offset).r;
            if (currentDepth > blockerDepth + uBias) {
                blockerSum += blockerDepth;
                blockerCount++;
            }
        }

        // Fully lit when no blockers are found
        if (blockerCount == 0) return 1.0;

        // Penumbra width from average blocker depth and receiver distance
        float avgBlockerDepth = blockerSum / float(blockerCount);
        float penumbra = (currentDepth - avgBlockerDepth) / avgBlockerDepth * uLightSize;

        // PCF pass with penumbra-scaled radius
        float shadow = 0.0;
        float filterRadius = penumbra * uPCFRadius;

        for (int i = 0; i < POISSON_SAMPLES; ++i) {
            vec2 offset = rot * poisson[i] * texelSize * filterRadius;
            float depth = texture(uShadowMap, proj.xy + offset).r;
            shadow += (currentDepth > depth + uBias) ? 0.0 : 1.0;
        }

        shadow /= float(POISSON_SAMPLES);
        return shadow;
    }

    // Hard Shadow
    if (uShadowMode == 0) {
        float closestDepth = texture(uShadowMap, proj.xy).r;
        float currentDepth = proj.z;
        float shadow = currentDepth > closestDepth + uBias ? 0.0 : 1.0;
        return shadow;
    }

    // 5 x 5 kernel PCF
    if (uShadowMode == 1) {
        float shadow = 0.0;
        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
        float currentDepth = proj.z;

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

    // 16 sample poisson PCF
    if (uShadowMode == 2) {
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

    // 64 sample poisson PCF
    if (uShadowMode == 3) {
        const vec2 poisson[64] = vec2[](
            vec2(-0.20247402,  0.15207597),
            vec2( 0.20149906, -0.86408825),
            vec2(-0.20054118,  0.72700785),
            vec2( 0.69409233,  0.12440118),
            vec2( 0.28097072, -0.29987406),
            vec2( 0.46928529, -0.45704720),
            vec2( 0.46028738,  0.24683710),
            vec2(-0.61937022, -0.19674099),
            vec2(-0.52993902,  0.42094412),
            vec2(-0.40316085, -0.77289486),
            vec2(-0.61481114, -0.66933754),
            vec2( 0.55550126, -0.68524955),
            vec2(-0.33689123, -0.50331671),
            vec2( 0.31626476, -0.62017842),
            vec2( 0.14490484,  0.46227538),
            vec2( 0.26529645,  0.66285755),
            vec2( 0.73385906,  0.51006072),
            vec2( 0.39544421,  0.49502017),
            vec2(-0.11624895, -0.26938865),
            vec2(-0.59811618,  0.67845403),
            vec2(-0.39357397, -0.05989137),
            vec2(-0.54517939, -0.42013838),
            vec2(-0.14568419,  0.46613697),
            vec2(-0.03260006, -0.89229408),
            vec2( 0.47928570, -0.19424873),
            vec2(-0.07627513,  0.91188786),
            vec2(-0.73246782,  0.52806083),
            vec2( 0.61436998,  0.39249953),
            vec2( 0.41038317, -0.81881637),
            vec2( 0.69935661, -0.55989159),
            vec2(-0.11781136, -0.48628885),
            vec2( 0.01548382,  0.67209550),
            vec2(-0.87231756,  0.32329777),
            vec2(-0.16840695, -0.07321553),
            vec2( 0.05567108,  0.05270869),
            vec2( 0.82874030, -0.32331492),
            vec2( 0.87016131, -0.11856777),
            vec2(-0.00132784,  0.28348242),
            vec2(-0.21698684, -0.88288154),
            vec2(-0.74772178, -0.52003833),
            vec2(-0.88747313, -0.09830558),
            vec2( 0.63532489, -0.33616059),
            vec2( 0.49452683,  0.02334738),
            vec2(-0.12092560, -0.68853546),
            vec2(-0.36246847,  0.82170153),
            vec2(-0.37417472,  0.60935246),
            vec2(-0.43501726,  0.18010042),
            vec2(-0.29625035,  0.36166212),
            vec2(-0.81693909, -0.29980647),
            vec2(-0.89268187,  0.11725455),
            vec2( 0.88906423,  0.07909920),
            vec2( 0.12665851, -0.41721299),
            vec2( 0.59386370,  0.66730540),
            vec2(-0.67195016,  0.26836286),
            vec2( 0.08532004, -0.17513422),
            vec2( 0.84824945,  0.30379112),
            vec2( 0.26598470,  0.27446100),
            vec2( 0.08171311, -0.66678970),
            vec2(-0.63940790,  0.04395555),
            vec2( 0.29414836,  0.01001298),
            vec2( 0.67396508, -0.10782049),
            vec2(-0.35232544, -0.27765891),
            vec2( 0.38869693,  0.83980514),
            vec2( 0.16636611,  0.88231662)
        );

        // Random per-fragment rotation to break spatial pattern repetition
        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        float angle = noise * 6.28318;
        float s = sin(angle);
        float c = cos(angle);
        mat2 rotation = mat2(c, -s, s, c);

        float shadow = 0.0;
        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
        float currentDepth = proj.z;

        for (int i = 0; i < 64; ++i) {
            vec2 offset = rotation * poisson[i] * texelSize * uPCFRadius;
            float depth = texture(uShadowMap, proj.xy + offset).r;
            shadow += (currentDepth > depth + uBias) ? 0.0 : 1.0;
        }

        shadow /= 64.0;
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

    // shadow mapping
    direct *= ShadowCalculation(world_pos, N, L);

    vec3 color = ambient + direct;

    // tone mapping
    color = toneMapReinhard(color);

    // gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
