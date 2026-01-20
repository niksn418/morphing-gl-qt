#include "Quad.h"
#include <QOpenGLFunctions>

namespace
{
    float vertices[] = {
        // positions  // texture coords
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
    };
} // namespace

Quad::Quad(std::shared_ptr<QOpenGLShaderProgram> program)
{
    vao_.create();
    vao_.bind();

    vbo_.create();
    vbo_.bind();
    vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_.allocate(vertices, sizeof(vertices));

    program->bind();
    program->enableAttributeArray(0);
    program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    program->enableAttributeArray(1);
    program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    program->release();

    vao_.release();
    vbo_.release();
}

void Quad::render(const QOpenGLContext & context)
{
    vao_.bind();
    context.functions()->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao_.release();
}
