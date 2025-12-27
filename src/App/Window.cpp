#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <array>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{

constexpr std::array<GLfloat, 21u> vertices = {
	0.0f, 0.707f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
	-0.5f, -0.5f, 0.f, 1.f, 0.f, 0.5f, 1.0f,
	0.5f, -0.5f, 0.f, 0.f, 1.f, 1.0f, 0.0f,
};
constexpr std::array<GLuint, 3u> indices = {0, 1, 2};

}// namespace

Window::Window() noexcept
{
	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");

	auto layout = new QVBoxLayout();
	layout->addWidget(fps, 1);

	setLayout(layout);

	timer_.start();
	deltaTimer_.start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	inputTimer_ = new QTimer(this);
	connect(inputTimer_, &QTimer::timeout, this, &Window::processInput);
	inputTimer_->start(16);

	connect(this, &Window::updateUI, [=, this] {
		fps->setText(formatFPS(ui_.fps));
	});
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
		texture_.reset();
		program_.reset();
		camera_.reset();
	}
}

void Window::onInit()
{
	camera_ = std::make_unique<Camera>();
	camera_->setPosition(QVector3D(0.0f, 2.0f, 0.0f));
	camera_->setYaw(0.0f);
	camera_->setPitch(-30.0f);
	camera_->setMoveSpeed(25.0f);
	camera_->setMouseSensitivity(0.1f);

	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vs");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/diffuse.fs");
	program_->link();

	// Create VAO object
	vao_.create();
	vao_.bind();

	// Create VBO
	vbo_.create();
	vbo_.bind();
	vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vbo_.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(GLfloat)));

	// Create IBO
	ibo_.create();
	ibo_.bind();
	ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	ibo_.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(GLuint)));

	texture_ = std::make_unique<QOpenGLTexture>(QImage(":/Textures/voronoi.png"));
	texture_->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
	texture_->setWrapMode(QOpenGLTexture::WrapMode::Repeat);

	// Bind attributes
	program_->bind();

	program_->enableAttributeArray(0);
	program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(7 * sizeof(GLfloat)));

	program_->enableAttributeArray(1);
	program_->setAttributeBuffer(1, GL_FLOAT, static_cast<int>(2 * sizeof(GLfloat)), 3,
								 static_cast<int>(7 * sizeof(GLfloat)));

	program_->enableAttributeArray(2);
	program_->setAttributeBuffer(2, GL_FLOAT, static_cast<int>(5 * sizeof(GLfloat)), 2,
								 static_cast<int>(7 * sizeof(GLfloat)));

	mvpUniform_ = program_->uniformLocation("mvp");

	// Release all
	program_->release();

	vao_.release();

	ibo_.release();
	vbo_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate MVP matrix
	model_.setToIdentity();
	model_.translate(0, 0, -2);
	const auto mvp = camera_->getViewProjectionMatrix() * model_;

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	// Update uniform value
	program_->setUniformValue(mvpUniform_, mvp);

	// Activate texture unit and bind texture
	glActiveTexture(GL_TEXTURE0);
	texture_->bind();

	// Draw
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

	// Release VAO and shader program
	texture_->release();
	vao_.release();
	program_->release();

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	if (camera_)
	{
		const auto aspect = static_cast<float>(width) / static_cast<float>(height);
		const auto zNear = 0.1f;
		const auto zFar = 100.0f;
		const auto fov = 60.0f;
		camera_->setPerspective(fov, aspect, zNear, zFar);
	}
}

void Window::processInput()
{
	if (!camera_)
		return;

	float deltaTime = deltaTimer_.restart() / 1000.0f;

	deltaTime = qMin(deltaTime, 0.1f);

	camera_->processKeyboardInput(pressedKeys_, deltaTime);
}

void Window::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		lastMousePos_ = event->pos();
		firstMouse_ = true;
	}
}

void Window::mouseMoveEvent(QMouseEvent * event)
{
	if (!camera_)
		return;

	if (event->buttons() & Qt::LeftButton)
	{
		if (firstMouse_)
		{
			lastMousePos_ = event->pos();
			firstMouse_ = false;
		}

		float xOffset = event->pos().x() - lastMousePos_.x();
		float yOffset = lastMousePos_.y() - event->pos().y();

		lastMousePos_ = event->pos();

		camera_->processMouseMovement(xOffset, yOffset);
	}
}

void Window::keyPressEvent(QKeyEvent * event)
{
	pressedKeys_.insert(event->key());
}

void Window::keyReleaseEvent(QKeyEvent * event)
{
	pressedKeys_.remove(event->key());
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{ std::move(callback) }
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}
	};
}
