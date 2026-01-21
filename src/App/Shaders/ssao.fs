#version 330 core

const int MAX_SSAO_SAMPLES = 128;
struct SSAOKernel
{
    vec3 samples[MAX_SSAO_SAMPLES];
    int size;
};

struct SSAOParams {
    SSAOKernel kernel;
    mat4 view;
    mat4 projection;
    bool hemisphere;
    bool smoothCheck;
    float radius;
};
float bias = 0.025;

uniform SSAOParams params;

uniform sampler2D posTexture;
uniform sampler2D normalTexture;
uniform sampler2D noiseTexture;

in vec2 vertTex;

out float out_col;

void main() {
    vec3 vertPos = texture(posTexture, vertTex).xyz;
    vertPos = (params.view * vec4(vertPos, 1.0)).xyz;

    mat3 TBN = mat3(1.0);
    if (params.hemisphere) {
        vec3 normal = texture(normalTexture, vertTex).xyz;
        normal = normalize(mat3(params.view) * normal);

        vec2 noiseScale = textureSize(normalTexture, 0) / textureSize(noiseTexture, 0);
        vec3 randomVec = texture(noiseTexture, vertTex * noiseScale).xyz;

        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        TBN = mat3(tangent, bitangent, normal);
    }

    float occlusion = 0.0;
    for (int i = 0; i < params.kernel.size; ++i) {
        vec3 samplePos = vertPos + TBN * params.kernel.samples[i];
        vec4 offset = params.projection * vec4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + vec2(0.5);

        vec3 offsetPos = texture(posTexture, offset.xy).xyz;
        offsetPos = (params.view * vec4(offsetPos, 1.0)).xyz;

        float sampleDepth = offsetPos.z;
        if (params.smoothCheck) {
            float rangeCheck = smoothstep(0.0, 1.0, params.radius / abs(vertPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        } else {
            if (abs(vertPos.z - sampleDepth) < params.radius)
                occlusion += step(samplePos.z, sampleDepth);
        }
    }

    occlusion = 1.0 - (occlusion / params.kernel.size);
    out_col = occlusion;
}
