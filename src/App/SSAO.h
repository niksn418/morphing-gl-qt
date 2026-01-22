#pragma once

#include "Camera.h"
#include "Quad.h"
#include <array>

#include "uniform_types.h"


inline constexpr unsigned int MAX_SSAO_SAMPLES = 128;

struct SSAOKernel
{
    std::array<QVector3D, MAX_SSAO_SAMPLES> samples;
    unsigned int size;
};

template <>
struct UniformLocType<SSAOKernel> {
	std::array<UniformLocType<QVector3D>, MAX_SSAO_SAMPLES> locs;
	UniformLocType<unsigned int> sizeLoc;
};

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

#define SSAO_SETTINGS_FIELDS(FIELD) \
    FIELD(bool, hemisphere)         \
    FIELD(bool, smoothCheck)        \
    FIELD(float, bias)              \
    FIELD(float, power)             \
    FIELD(float, kernelRadius)      \
	FIELD(float, sampleRadius)
define_uniform_struct(SSAOSettings, SSAO_SETTINGS_FIELDS)

#define SSAO_FIELDS(FIELD)          \
	FIELD(SSAOKernel, kernel)       \
	FIELD(QMatrix4x4, view)         \
	FIELD(QMatrix4x4, projection)   \
	FIELD(SSAOSettings, settings)
define_uniform_struct(SSAOParams, SSAO_FIELDS)

class SSAO
{
public:
    SSAO(std::shared_ptr<QOpenGLShaderProgram> program, const QOpenGLContext & context);
    void render(const Camera & camera, const QOpenGLContext & context,
                GLuint posTexId, GLuint normTexId);

    void setSamplesNum(unsigned int n);

    static SSAOSettings defaultSettings();
	SSAOSettings settings = defaultSettings();

private:
    const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
    const GLuint noise_;

    UniformLocType<SSAOParams> paramsUniform_;
    UniformLocType<float> aspectRatioUniform_;
    UniformLocType<float> fovHalfTangentUniform_;

    SSAOKernel kernel_;
    float kernelRadius_ = 1.f;
    float sampleRadius_ = 1.f;
    bool hemisphere_ = true;
    bool smoothCheck_ = true;
	Quad quad_;
};

class SSAOBlur
{
public:
    SSAOBlur(std::shared_ptr<QOpenGLShaderProgram> program);
    void render(const QOpenGLContext & context, GLuint ssaoTexId);

private:
    const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
	Quad quad_;
};
