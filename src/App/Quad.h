#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <memory>

class Quad
{
public:
    Quad(std::shared_ptr<QOpenGLShaderProgram> program);
    void render(const QOpenGLContext & context);

private:
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_;
};
