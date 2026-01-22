#version 330 core

uniform float aspectRatio;
uniform float fovHalfTangent;

layout(location=0) in vec2 pos;
layout(location=1) in vec2 tex;

out vec2 vertTex;
out vec2 viewRay;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vertTex = tex;
    viewRay.x = pos.x * aspectRatio * fovHalfTangent;
    viewRay.y = pos.y * fovHalfTangent;
}
