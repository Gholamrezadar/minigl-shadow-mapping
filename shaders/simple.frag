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

    // 5 x 5 kernel PCF
    if(uPCF && !uPCFPoisson) {
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

    // 64 sample poisson PCF
    if (uPCF && uPCFPoisson && false) {
        const vec2 poisson[64] = vec2[](
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
            vec2( 0.14383161, -0.14100790),
            vec2(-0.49379003,  0.66634812),
            vec2( 0.60162562,  0.49561076),
            vec2(-0.71243374, -0.20872621),
            vec2( 0.28540695,  0.69900621),
            vec2(-0.13227908,  0.45115040),
            vec2( 0.73519237, -0.30561990),
            vec2(-0.55257166,  0.84144138),
            vec2( 0.17429148, -0.53865623),
            vec2(-0.33698258, -0.74039441),
            vec2( 0.83574929,  0.36417668),
            vec2(-0.07359774, -0.59874681),
            vec2( 0.48753497, -0.67865692),
            vec2(-0.62458476,  0.32564970),
            vec2( 0.11172939,  0.96217913),
            vec2(-0.86433626, -0.55483060),
            vec2( 0.65284279, -0.59239101),
            vec2(-0.44315100,  0.15528323),
            vec2( 0.36818503,  0.87795861),
            vec2(-0.27045417, -0.17016055),
            vec2( 0.91781567,  0.05419345),
            vec2(-0.58493742,  0.54528481),
            vec2( 0.39961623, -0.29883191),
            vec2(-0.16374069,  0.74668698),
            vec2( 0.69810146,  0.71490699),
            vec2(-0.74765376, -0.43267740),
            vec2( 0.22192483,  0.34475912),
            vec2(-0.92772232,  0.20716491),
            vec2( 0.50721456, -0.11855571),
            vec2(-0.34265552, -0.92751162),
            vec2( 0.76888658, -0.49029623),
            vec2(-0.61532742,  0.70419001),
            vec2( 0.04871579, -0.76987384),
            vec2(-0.20338682,  0.22883975),
            vec2( 0.87278538, -0.15594491),
            vec2(-0.46718676, -0.60497093),
            vec2( 0.31893521,  0.56798780),
            vec2(-0.80059657,  0.07638979),
            vec2( 0.58782800,  0.80990577),
            vec2(-0.05249844, -0.38819902),
            vec2( 0.42846580, -0.82312202),
            vec2(-0.67844507,  0.12946322),
            vec2( 0.24631582, -0.05148779),
            vec2(-0.52793116,  0.97650756),
            vec2( 0.71635002, -0.88335399),
            vec2(-0.89403088, -0.10590116),
            vec2( 0.08632727,  0.60878550),
            vec2(-0.33085176, -0.50374897),
            vec2( 0.95920592,  0.44419367)
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

    // PCSS
    if (false)
    {
        float currentDepth = proj.z;

        vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));

        // ---------------------------------------
        // STEP 1: BLOCKER SEARCH
        // ---------------------------------------
        int blockerCount = 0;
        float avgBlockerDepth = 0.0;

        // small fixed kernel for blocker search
        const int BLOCKER_SAMPLES = 16;

        float blockerRadius = uPCFRadius * 2.0;

        for (int i = 0; i < BLOCKER_SAMPLES; ++i)
        {
            // simple uniform disk sampling (can be improved later)
            float angle = float(i) * 6.2831853 / float(BLOCKER_SAMPLES);
            vec2 dir = vec2(cos(angle), sin(angle));

            vec2 offset = dir * texelSize * blockerRadius;

            float depth = texture(uShadowMap, proj.xy + offset).r;

            // occluder test
            if (depth < currentDepth - uBias)
            {
                avgBlockerDepth += depth;
                blockerCount++;
            }
        }

        // no blockers → fully lit
        if (blockerCount == 0)
            return 1.0;

        avgBlockerDepth /= float(blockerCount);

        // ---------------------------------------
        // STEP 2: PENUMBRA ESTIMATION
        // ---------------------------------------
        float penumbra = (currentDepth - avgBlockerDepth) / avgBlockerDepth;

        // scale factor to tune softness
        penumbra *= uLightSize;

        float filterRadius = penumbra * uPCFRadius;

        // ---------------------------------------
        // STEP 3: PCF WITH DYNAMIC RADIUS
        // ---------------------------------------
        const int PCSS_SAMPLES = 32;

        float shadow = 0.0;

        for (int i = 0; i < PCSS_SAMPLES; ++i)
        {
            float angle = float(i) * 6.2831853 / float(PCSS_SAMPLES);
            vec2 dir = vec2(cos(angle), sin(angle));

            vec2 offset = dir * texelSize * filterRadius;

            float depth = texture(uShadowMap, proj.xy + offset).r;

            shadow += (currentDepth > depth + uBias) ? 0.0 : 1.0;
        }

        shadow /= float(PCSS_SAMPLES);

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