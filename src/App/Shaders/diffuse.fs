#version 330 core

struct Light
{
	vec3 color;
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
};

struct DirectionalLight
{
	vec3 direction;
	Light light;
};

struct PointLight
{
	vec3 position;
	Light light;

	float linear;
	float quadratic;
};

struct SpotLight
{
	PointLight bulb;
	vec3 direction;
	float cutOff;
	float outerCutOff;
};

struct Lights
{
	DirectionalLight dirLight;
	PointLight pointLight;
	SpotLight spotLight;
};

uniform sampler2D tex_2d;
uniform Lights lights;
uniform vec3 viewPos;

in vec3 vert_pos;
in vec3 vert_norm;
in vec2 vert_tex;

out vec4 out_col;

mat3 phongLighting(vec3 norm, vec3 viewDir, Light light, vec3 lightDir) {
	vec3 ambient = light.ambientStrength * light.color;

	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuseStrength * diff * light.color;

	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = light.specularStrength * spec * light.color;

	return mat3(ambient, diffuse, specular);
}

mat3 dirLightColor(DirectionalLight light, vec3 norm, vec3 viewDir) {
	return phongLighting(norm, viewDir, light.light, normalize(-light.direction));
}

mat3 pointLightColor(PointLight light, vec3 norm, vec3 viewDir) {
	vec3 lightDir = light.position - vert_pos;
	float dist = length(lightDir);
	mat3 phong_colors = phongLighting(norm, viewDir, light.light, normalize(lightDir));
	float attenuation = 1 + light.linear * dist + light.quadratic * (dist * dist);
	return phong_colors / attenuation;
}

mat3 spotLightColor(SpotLight light, vec3 norm, vec3 viewDir) {
	mat3 phong_colors = pointLightColor(light.bulb, norm, viewDir);
	float theta = dot(normalize(light.bulb.position - vert_pos), normalize(-light.direction));
	float epsilon = (light.cutOff - light.outerCutOff);
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	phong_colors[1] *= intensity;
	phong_colors[2] *= intensity;
	return phong_colors;
}

void main() {
	vec3 norm = normalize(vert_norm);
	vec3 viewDir = normalize(viewPos - vert_pos);

	mat3 lightColors = dirLightColor(lights.dirLight, norm, viewDir);
	lightColors += pointLightColor(lights.pointLight, norm, viewDir);
	lightColors += spotLightColor(lights.spotLight, norm, viewDir);
	vec3 lightColor = lightColors[0] + lightColors[1] + lightColors[2];

	vec4 texColor = texture(tex_2d, vert_tex);
	vec3 result = lightColor * texColor.rgb;
	out_col = vec4(result, texColor.a);
}