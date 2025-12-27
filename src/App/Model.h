#pragma once

#include "Camera.h"
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <vector>

struct Vertex {
	float position[3];
	float normal[3];
	float texCoord[2];
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	int textureIndex = -1;
};

class Model
{
public:
	Model(std::shared_ptr<QOpenGLShaderProgram> program);
	~Model();

	bool loadFromGLTF(const QString & filePath);

	void render(const Camera & camera, const QOpenGLContext & context);

	const std::vector<Mesh> & getMeshes() const { return meshes_; }
	bool isLoaded() const { return !meshes_.empty(); }

	void setPosition(const QVector3D & position);
	void setRotation(const QVector3D & rotation);
	void setScale(const QVector3D & scale);

	const QVector3D & getPosition() const { return position_; }
	const QVector3D & getRotation() const { return rotation_; }
	const QVector3D & getScale() const { return scale_; }

	const QMatrix4x4 & getTransform() const;

	bool isVisible() const { return visible_; }
	void setVisible(bool visible) { visible_ = visible; }

private:
	void setupMeshBuffers();
	void cleanupResources();
	void updateTransform() const;
	void markTransformDirty() { transformDirty_ = true; }

	QVector3D position_{0.0f, 0.0f, 0.0f};
	QVector3D rotation_{0.0f, 0.0f, 0.0f};
	QVector3D scale_{1.0f, 1.0f, 1.0f};

	mutable QMatrix4x4 transform_;
	mutable bool transformDirty_ = true;

	bool visible_ = true;

	std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
	std::vector<std::unique_ptr<QOpenGLTexture>> textures_;
	std::vector<Mesh> meshes_;

	std::vector<std::unique_ptr<QOpenGLBuffer>> vbos_;
	std::vector<std::unique_ptr<QOpenGLBuffer>> ibos_;
	std::vector<std::unique_ptr<QOpenGLVertexArrayObject>> vaos_;

	GLint mvpUniform_ = -1;
};
