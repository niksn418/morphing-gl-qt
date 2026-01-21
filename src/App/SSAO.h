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
									QString varName);
template <>
void setUniformValue<SSAOKernel>(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
							     const UniformLocType<SSAOKernel> & locs,
                                 const SSAOKernel & value);


#define SSAO_FIELDS(FIELD)          \
	FIELD(SSAOKernel, kernel)       \
	FIELD(QMatrix4x4, view)         \
	FIELD(QMatrix4x4, projection)   \
	FIELD(float, radius)
define_uniform_struct(SSAOParams, SSAO_FIELDS)

class SSAO
{
public:
    SSAO(std::shared_ptr<QOpenGLShaderProgram> program);
    void render(const Camera & camera, const QOpenGLContext & context,
                GLuint posTexId);

    void setSamplesNum(unsigned int n);
    void setRadius(float radius) { radius_ = radius; }

private:
    const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;

    UniformLocType<SSAOParams> paramsUniform_;

    SSAOKernel kernel_;
    float radius_ = 1.f;
	Quad quad_;
};
