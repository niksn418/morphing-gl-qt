#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 tex;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normalMatrix;

uniform vec3 bBoxCenter;
uniform float bBoxRadius;
uniform float morphing;

out vec3 vert_pos;
out vec3 vert_norm;
out vec2 vert_tex;

void main() {
	vec3 vertRadius = pos - bBoxCenter;
	vec3 vertRadiusNorm = normalize(vertRadius);
	vec3 morphedPos = bBoxCenter + mix(vertRadius, vertRadiusNorm * bBoxRadius, morphing);
	vec3 morphedNormal = mix(normal, vertRadiusNorm, morphing);

	vert_pos = vec3(model * vec4(morphedPos, 1.0));
	vert_norm = normalMatrix * morphedNormal;
	vert_tex = tex;
	gl_Position = mvp * vec4(morphedPos, 1.0);
}
