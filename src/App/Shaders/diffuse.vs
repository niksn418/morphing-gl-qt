#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 tex;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 vert_pos;
out vec3 vert_norm;
out vec2 vert_tex;

void main() {
	vert_pos = vec3(model * vec4(pos, 1.0));
    vert_norm = normalMatrix * normal;
	vert_tex = tex;
	gl_Position = mvp * vec4(pos, 1.0);
}
