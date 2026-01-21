#pragma once

#include "Camera.h"
#include "Quad.h"
#include <QOpenGLShaderProgram>

#include <memory>
#include <optional>

#include "uniform_types.h"

#define Light_FIELDS(FIELD)       \
	FIELD(QVector3D, color)       \
	FIELD(float, ambientStrength) \
	FIELD(float, diffuseStrength) \
	FIELD(float, specularStrength)
define_uniform_struct(Light, Light_FIELDS)


#define DirectionalLight_FIELDS(FIELD)	\
	FIELD(QVector3D, direction)			\
	FIELD(Light, light)
define_uniform_struct(DirectionalLight, DirectionalLight_FIELDS)


#define PointLight_FIELDS(FIELD)    \
	FIELD(QVector3D, position)      \
	FIELD(Light, light)             \
	/* constant part is always 1 */ \
	FIELD(float, linear)            \
	FIELD(float, quadratic)
define_uniform_struct(PointLight, PointLight_FIELDS)


#define SpotLight_FIELDS(FIELD) \
	FIELD(PointLight, bulb)     \
	FIELD(QVector3D, direction) \
	FIELD(float, cutOff)        \
	FIELD(float, outerCutOff)
define_uniform_struct(SpotLight, SpotLight_FIELDS)


#define Lights_FIELDS(FIELD)          \
	FIELD(DirectionalLight, dirLight) \
	FIELD(PointLight, pointLight)     \
	FIELD(SpotLight, spotLight)
define_uniform_struct(Lights, Lights_FIELDS)


class Lighting
{
public:
	Lighting(std::shared_ptr<QOpenGLShaderProgram> program);
	~Lighting() = default;

	void render(const Camera & camera, const QOpenGLContext & context,
				GLuint posTexId, GLuint normalTexId, GLuint colorTexId,
				std::optional<GLuint> ssaoTexId);

	static Lights defaultLights();
	Lights lights_ = defaultLights();

private:
	const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;

	UniformLocType<Lights> lightsUniform_;
	UniformLocType<QVector3D> viewPosUniform_;
	UniformLocType<bool> ssaoUniform_;

	Quad quad_;
};
