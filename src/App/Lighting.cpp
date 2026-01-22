#include "Lighting.h"
#include <QOpenGLFunctions>
#include <QtMath>
#include <QVector3D>

Lights Lighting::defaultLights() {
	return {
		.dirLight{
			.direction{1.f, -1.f, 1.f},
			.light{
				.color{1.f, 1.f, .5f},
				.ambientStrength = .75f,
				.diffuseStrength = .4f,
				.specularStrength = .5f
			}
		},
		.pointLight{
			.position{0.f, 10.f, 0.f},
			.light{
				.color{1.f, 1.f, 1.f},
				.ambientStrength = .05f,
				.diffuseStrength = .8f,
				.specularStrength = 1.f
			},
			.linear = .35f,
			.quadratic = .44f
		},
		.spotLight{
			.bulb{
				.position{2.f, 2.f, 2.f},
				.light{
					.color{1.f, 1.f, 1.f},
					.ambientStrength = 0.f,
					.diffuseStrength = 1.f,
					.specularStrength = 1.f
				},
				.linear = .09f,
				.quadratic = .032f
			},
			.direction{-2.f, -2.f, -2.f},
			.cutOff{std::cos(qDegreesToRadians(12.5f))},
			.outerCutOff{std::cos(qDegreesToRadians(15.f))},
		}
	};
}

Lighting::Lighting(std::shared_ptr<QOpenGLShaderProgram> program)
	: shaderProgram_(program)
{
	program->bind();
	lightsUniform_ = bindUniform<Lights>(program, "lights");
	viewPosUniform_ = bindUniform<QVector3D>(program, "viewPos");
	ssaoUniform_ = bindUniform<bool>(program, "useSSAO");
	program->setUniformValue("ssaoTexture", 1);
	program->release();
}

void Lighting::render(const Camera & camera, const QOpenGLContext & context,
					  std::optional<GLuint> ssaoTexId)
{
	shaderProgram_->bind();
	setUniformValue(shaderProgram_, lightsUniform_, lights_);
	setUniformValue(shaderProgram_, viewPosUniform_, camera.getPosition());
	setUniformValue(shaderProgram_, ssaoUniform_, ssaoTexId.has_value());
	if (ssaoTexId.has_value())
	{
		context.functions()->glActiveTexture(GL_TEXTURE1);
		context.functions()->glBindTexture(GL_TEXTURE_2D, ssaoTexId.value());
	}
	shaderProgram_->release();
}
