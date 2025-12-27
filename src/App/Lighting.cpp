#include "Camera.h"
#include "Lighting.h"
#include <QVector3D>

Lighting::Lighting(std::shared_ptr<QOpenGLShaderProgram> program)
	: shaderProgram_(program)
{
	program->bind();
	lightPosUniform_ = program->uniformLocation("lightPos"); // TODO: check
	lightColorUniform_ = program->uniformLocation("lightColor"); // TODO: check
	viewPosUniform_ = program->uniformLocation("viewPos"); // TODO: check
	program->release();
}

void Lighting::render(const Camera & camera)
{
	shaderProgram_->bind();
	shaderProgram_->setUniformValue(lightPosUniform_, QVector3D(0.0f, 10.0f, 0.0f));
	shaderProgram_->setUniformValue(lightColorUniform_, QVector3D(1.0f, 1.0f, 1.0f));
	shaderProgram_->setUniformValue(viewPosUniform_, camera.getPosition());
	shaderProgram_->release();
}
