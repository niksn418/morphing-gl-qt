#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 tex;

struct ModelUniform {
	mat4 mvp;
	mat4 transform;
	mat3 normalMatrix;

	vec3 bBoxCenter;
	float bBoxRadius;
	float morphing;
};
uniform ModelUniform model;

out vec3 vert_pos;
out vec3 vert_norm;
out vec2 vert_tex;

void main() {
	vec3 v = pos - model.bBoxCenter;
	vec3 r = normalize(v);
	float t = model.morphing;
	vec3 morphedPos = model.bBoxCenter + mix(v, r * model.bBoxRadius, t);
	vec3 morphedNormal = normalize(mix(
		normal * length(v),
		dot(r, normal) * r * model.bBoxRadius,
		t
	));

	vert_pos = vec3(model.transform * vec4(morphedPos, 1.0));
	vert_norm = model.normalMatrix * morphedNormal;
	vert_tex = tex;
	gl_Position = model.mvp * vec4(morphedPos, 1.0);
}
