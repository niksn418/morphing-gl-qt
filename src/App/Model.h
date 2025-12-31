#pragma once

#include "Camera.h"
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <vector>

#include "uniform_types.h"

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

#define ModelUniform_FIELDS(FIELD)  \
	FIELD(QMatrix4x4, mvp)          \
	FIELD(QMatrix4x4, transform)    \
	FIELD(QMatrix3x3, normalMatrix) \
	FIELD(QVector3D, bBoxCenter)    \
	FIELD(float, bBoxRadius)        \
	FIELD(float, morphing)
define_uniform_struct(ModelUniform, ModelUniform_FIELDS)

#define ModelTexture_FIELDS(FIELD)  \
	FIELD(QVector3D, color)  \
	FIELD(bool, use)
define_uniform_struct(FallbackTexture, ModelTexture_FIELDS)

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
	void setScale(float scale);
	void setMorphing(float morphing);
	void useFallbackTexture(bool use) { fallbackTexture_.use = use; }
	void useFallbackTexture(QVector3D color) { fallbackTexture_.color = color; }

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
	float morphing_ = 0.f;
	FallbackTexture fallbackTexture_{};

	mutable QMatrix4x4 transform_;
	mutable bool transformDirty_ = true;

	bool visible_ = true;

	const std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
	std::vector<std::unique_ptr<QOpenGLTexture>> textures_;
	std::vector<Mesh> meshes_;
	struct {
		QVector3D min;
		QVector3D max;
	} bounding_box;

	std::vector<std::unique_ptr<QOpenGLBuffer>> vbos_;
	std::vector<std::unique_ptr<QOpenGLBuffer>> ibos_;
	std::vector<std::unique_ptr<QOpenGLVertexArrayObject>> vaos_;

	UniformLocType<ModelUniform> modelUniform_;
	UniformLocType<FallbackTexture> fallbackTextureUniform_;
};
