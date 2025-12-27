#include "Camera.h"
#include "Lighting.h"
#include "debug.h"
#include <QVector3D>

Lighting::Lighting(std::shared_ptr<QOpenGLShaderProgram> program)
	: shaderProgram_(program)
{
	program->bind();
	lightPosUniform_ = program->uniformLocation("lightPos");
	check(lightPosUniform_ != -1);
	lightColorUniform_ = program->uniformLocation("lightColor");
	check(lightColorUniform_ != -1);
	viewPosUniform_ = program->uniformLocation("viewPos");
	check(viewPosUniform_ != -1);
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
