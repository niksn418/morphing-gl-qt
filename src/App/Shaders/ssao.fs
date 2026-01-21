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
    float radius;
};

uniform SSAOParams params;

uniform sampler2D posTexture;

in vec2 vertTex;

out float out_col;

void main() {
    vec3 vertPos = texture(posTexture, vertTex).xyz;
    vertPos = (params.view * vec4(vertPos, 1.0)).xyz;

    float occlusion = 0.0;
    for (int i = 0; i < params.kernel.size; ++i) {
        vec3 samplePos = vertPos + params.kernel.samples[i];
        vec4 offset = params.projection * vec4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + vec2(0.5);

        vec3 offsetPos = texture(posTexture, offset.xy).xyz;
        offsetPos = (params.view * vec4(offsetPos, 1.0)).xyz;

        float sampleDepth = offsetPos.z;
        if (abs(vertPos.z - sampleDepth) < params.radius)
            occlusion += step(samplePos.z, sampleDepth);
    }

    occlusion = 1.0 - (occlusion / params.kernel.size);
    out_col = occlusion;
}
