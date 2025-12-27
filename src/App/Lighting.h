#pragma once

#include <QOpenGLShaderProgram>

#include <memory>

class Lighting
{
public:
	Lighting(std::shared_ptr<QOpenGLShaderProgram> program);
	~Lighting() = default;

	void render(const Camera & camera);

private:
	const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;

	GLint lightPosUniform_ = -1;
	GLint lightColorUniform_ = -1;
	GLint viewPosUniform_ = -1;
};
