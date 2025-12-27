#version 330 core

uniform sampler2D tex_2d;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

in vec3 vert_pos;
in vec3 vert_norm;
in vec2 vert_tex;

out vec4 out_col;

void main() {
	vec3 norm = normalize(vert_norm);

	float ambientStrength = 0.5;
	vec3 ambient = ambientStrength * lightColor;

	vec3 lightDir = normalize(lightPos - vert_pos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	float specularStrength = 0.5;
	vec3 viewDir = normalize(viewPos - vert_pos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	vec4 texColor = texture(tex_2d, vert_tex);
	vec3 result = (ambient + diffuse + specular) * texColor.rgb;
	out_col = vec4(result, texColor.a);
}