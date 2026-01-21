#include "SSAO.h"
#include <QOpenGLFunctions>
#include <random>

namespace
{
    float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }
} // namespace

template <>
UniformLocType<SSAOKernel> bindUniform<SSAOKernel>(
                                    std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
									QString varName)
{
	UniformLocType<SSAOKernel> loc;
	loc.sizeLoc = bindUniform<unsigned int>(shaderProgram, varName + ".size");
	for (unsigned int i = 0; i < MAX_SSAO_SAMPLES; ++i)
	{
		loc.locs[i] = bindUniform<QVector3D>(shaderProgram,
                                             varName + ".samples[" + QString::number(i) + "]");
	}
	return loc;
}

template <>
void setUniformValue<SSAOKernel>(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
							     const UniformLocType<SSAOKernel> & locs,
                                 const SSAOKernel & value)
{
    setUniformValue(shaderProgram, locs.sizeLoc, value.size);
	for (unsigned int i = 0; i < value.size; ++i)
	{
        setUniformValue(shaderProgram, locs.locs[i], value.samples[i]);
	}
}

SSAO::SSAO(std::shared_ptr<QOpenGLShaderProgram> program)
    : shaderProgram_(program)
    , quad_(program)
{
    setSamplesNum(MAX_SSAO_SAMPLES);
    program->bind();
	paramsUniform_ = bindUniform<SSAOParams>(program, "params");
	program->setUniformValue("posTexture", 0);
	program->release();
}

void SSAO::render(const Camera & camera, const QOpenGLContext & context,
             GLuint posTexId)
{
    shaderProgram_->bind();
	setUniformValue(shaderProgram_, paramsUniform_, SSAOParams {
        .kernel = kernel_,
        .view = camera.getViewMatrix(),
        .projection = camera.getProjectionMatrix(),
        .radius = radius_
    });
	context.functions()->glActiveTexture(GL_TEXTURE0);
	context.functions()->glBindTexture(GL_TEXTURE_2D, posTexId);
	quad_.render(context);
	shaderProgram_->release();
}

void SSAO::setSamplesNum(unsigned int n)
{
    std::uniform_real_distribution<float> rnd(0.f, 1.f);
    std::default_random_engine gen(42);
    for (unsigned int i = 0; i < n; ++i)
    {
        auto sample = QVector3D(
            2.f * rnd(gen) - 1.f,
            2.f * rnd(gen) - 1.f,
            2.f * rnd(gen) - 1.f
        );
        sample.normalize();
        sample *= rnd(gen);

        float scale = float(i) / n;
        scale = lerp(.1f, 1.f, scale * scale);
        sample *= scale;
        kernel_.samples[i] = sample;
    }
    kernel_.size = n;
}
