#include "SSAO.h"
#include <QOpenGLFunctions>
#include <QtMath>
#include <random>

namespace
{
    float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    auto genNoise()
    {
        std::uniform_real_distribution<float> rnd(0.f, 1.f);
        std::default_random_engine gen(42);
        std::array<std::array<float, 3>, 16> noise;
        for (unsigned int i = 0; i < 16; i++)
        {
            QVector3D vec(2.f * rnd(gen) - 1.f, 2.f * rnd(gen) - 1.f, 0.f);
            vec.normalize();
            noise[i][0] = vec.x();
            noise[i][1] = vec.y();
            noise[i][2] = vec.z();
        }
        return noise;
    }

    GLuint createNoiseTexture(const QOpenGLContext & context)
    {
        GLuint noiseTexture;
        auto noise = genNoise();
        context.functions()->glGenTextures(1, &noiseTexture);
        context.functions()->glBindTexture(GL_TEXTURE_2D, noiseTexture);
        context.functions()->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4,
                                          0, GL_RGB, GL_FLOAT, &noise[0]);
        context.functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        context.functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        context.functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        context.functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return noiseTexture;
    }
} // namespace

SSAOSettings SSAO::defaultSettings()
{
    return SSAOSettings{
        .hemisphere = true,
        .smoothCheck = true,
        .bias = 0.025,
        .power = 1.0,
        .kernelRadius = 1.0,
        .sampleRadius = 1.0
    };
}

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

SSAO::SSAO(std::shared_ptr<QOpenGLShaderProgram> program, const QOpenGLContext & context)
    : shaderProgram_(program)
    , noise_(createNoiseTexture(context))
    , quad_(program)
{
    setSamplesNum(MAX_SSAO_SAMPLES);
    program->bind();
	paramsUniform_ = bindUniform<SSAOParams>(program, "params");
    aspectRatioUniform_ = bindUniform<float>(program, "aspectRatio");
    fovHalfTangentUniform_ = bindUniform<float>(program, "fovHalfTangent");
    program->setUniformValue("depthTexture", 0);
    program->setUniformValue("normalTexture", 1);
    program->setUniformValue("noiseTexture", 2);
	program->release();
}

void SSAO::render(const Camera & camera, const QOpenGLContext & context,
                  GLuint depthTexId, GLuint normTexId)
{
    shaderProgram_->bind();
	setUniformValue(shaderProgram_, paramsUniform_, SSAOParams {
        .kernel = kernel_,
        .view = camera.getViewMatrix(),
        .projection = camera.getProjectionMatrix(),
        .settings = settings
    });
    setUniformValue(shaderProgram_, aspectRatioUniform_, camera.getAspectRatio());
    float fovHalfTan = qTan(qDegreesToRadians(camera.getFOV() / 2));
    setUniformValue(shaderProgram_, fovHalfTangentUniform_, fovHalfTan);
	context.functions()->glActiveTexture(GL_TEXTURE0);
	context.functions()->glBindTexture(GL_TEXTURE_2D, depthTexId);
    context.functions()->glActiveTexture(GL_TEXTURE1);
	context.functions()->glBindTexture(GL_TEXTURE_2D, normTexId);
    context.functions()->glActiveTexture(GL_TEXTURE2);
	context.functions()->glBindTexture(GL_TEXTURE_2D, noise_);
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

SSAOBlur::SSAOBlur(std::shared_ptr<QOpenGLShaderProgram> program)
    : shaderProgram_(program)
    , quad_(program)
{
    program->bind();
	program->setUniformValue("ssaoTexture", 0);
	program->release();
}

void SSAOBlur::render(const QOpenGLContext & context, GLuint ssaoTexId)
{
    shaderProgram_->bind();
	context.functions()->glActiveTexture(GL_TEXTURE0);
	context.functions()->glBindTexture(GL_TEXTURE_2D, ssaoTexId);
	quad_.render(context);
	shaderProgram_->release();
}
