#version 330 core

layout(location=0) out vec3 normal;

in vec3 vertPos;
in vec3 vertNorm;
in vec2 vertTex;

void main() {
    normal = normalize(vertNorm);
}