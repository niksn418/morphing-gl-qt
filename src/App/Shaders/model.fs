#version 330 core

struct FallbackTexture
{
	vec3 color;
	bool use;
};

uniform sampler2D tex_2d;
uniform FallbackTexture fallbackTexture;

layout(location=0) out vec3 pos;
layout(location=1) out vec3 normal;
layout(location=2) out vec4 color;

in vec3 vert_pos;
in vec3 vert_norm;
in vec2 vert_tex;

void main() {
    pos = vert_pos;
    normal = normalize(vert_norm);
    color = fallbackTexture.use
            ? vec4(fallbackTexture.color, 0.0)
            : texture(tex_2d, vert_tex).rgba;
}