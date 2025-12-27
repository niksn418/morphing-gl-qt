#include "Window.h"
#include "debug.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <cmath>

Window::Window() noexcept
{
	settingsUi_ = std::make_unique<QGroupBox>("Settings", this);
	settingsUi_->setAlignment(Qt::AlignHCenter);
	settingsUi_->setStyleSheet(
		"QGroupBox {"
			"background-color: rgba(128, 128, 128, 128);"
			"font-weight: bold;"
			"font-size: 20pt;"
		"}"
	);
	settingsUi_->setContentsMargins(0, 0, 0, 0);
	settingsUi_->setVisible(settingsOpen_);

	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");

	auto layout = new QVBoxLayout();
	layout->addWidget(fps);
	layout->addStretch(1);

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
		model_.reset();
		lighting_.reset();
		camera_.reset();
		program_.reset();
		settingsUi_.reset();
	}
}

void Window::onInit()
{
	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	check(!!program_);
	check(program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vs"));
	check(program_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/diffuse.fs"));
	check(program_->link());

	camera_ = std::make_unique<Camera>();
	camera_->setPosition(QVector3D(0.0f, 2.0f, 0.0f));
	camera_->setYaw(0.0f);
	camera_->setPitch(-30.0f);
	camera_->setMoveSpeed(25.0f);
	camera_->setMouseSensitivity(0.1f);

	lighting_ = std::make_unique<Lighting>(program_);

	model_ = std::make_unique<Model>(program_);
	check(model_->loadFromGLTF(":/Models/sponza.glb"));
	model_->setScale(QVector3D(0.01f, 0.01f, 0.01f));
	model_->setPosition(QVector3D(0.0f, 0.0f, 0.0f));

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

	const auto& camera = *camera_;
	const auto& glContext = *context();
	model_->render(camera, glContext);
	lighting_->render(camera);

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
	settingsUi_->setGeometry(rect());

	const auto aspect = static_cast<float>(width) / static_cast<float>(height);
	const auto zNear = 0.1f;
	const auto zFar = 100.0f;
	const auto fov = 60.0f;
	camera_->setPerspective(fov, aspect, zNear, zFar);
}

void Window::processInput()
{
	float deltaTime = deltaTimer_.restart() / 1000.0f;

	deltaTime = qMin(deltaTime, 0.1f);

	if (settingsOpen_) return;
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
	if (settingsOpen_)
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
	if (event->key() == Qt::Key_Escape) {
		settingsOpen_ ^= true;
		settingsUi_->setVisible(settingsOpen_);
	}
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
