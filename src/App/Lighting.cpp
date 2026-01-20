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
				.ambientStrength = .05f,
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
	, quad(program)
{
	program->bind();
	lightsUniform_ = bindUniform<Lights>(program, "lights");
	viewPosUniform_ = bindUniform<QVector3D>(program, "viewPos");
	program->setUniformValue("posTexture", 0);
	program->setUniformValue("normalTexture", 1);
	program->setUniformValue("colorTexture", 2);
	program->release();
}

void Lighting::render(const Camera & camera, const QOpenGLContext & context,
					  GLuint posTexId, GLuint normalTexId, GLuint colorTexId)
{
	shaderProgram_->bind();
	setUniformValue(shaderProgram_, lightsUniform_, lights_);
	setUniformValue(shaderProgram_, viewPosUniform_, camera.getPosition());
	context.functions()->glActiveTexture(GL_TEXTURE0);
	context.functions()->glBindTexture(GL_TEXTURE_2D, posTexId);
	context.functions()->glActiveTexture(GL_TEXTURE1);
	context.functions()->glBindTexture(GL_TEXTURE_2D, normalTexId);
	context.functions()->glActiveTexture(GL_TEXTURE2);
	context.functions()->glBindTexture(GL_TEXTURE_2D, colorTexId);
	quad.render(context);
	shaderProgram_->release();
}
