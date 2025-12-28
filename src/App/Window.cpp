#include "Window.h"
#include "utils.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <cmath>

using namespace window_internals;

Window::Window() noexcept
	: cameraId_(DEFAULT), settingsUi_(initSettingsUi())
{
	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");

	auto layout = new QVBoxLayout();
	layout->addWidget(fps);
	layout->addStretch();

	setLayout(layout);

	timer_.start();
	deltaTimer_.start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

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
	camera_->setPosition(QVector3D(-1.0f, 2.0f, 0.0f));
	camera_->setYaw(0.0f);
	camera_->setPitch(-30.0f);
	camera_->setMoveSpeed(25.0f);
	camera_->setMouseSensitivity(0.1f);

	lighting_ = std::make_unique<Lighting>(program_);

	model_ = std::make_unique<Model>(program_);
	reloadModel();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::reloadModel()
{
	const auto & model_variant = MODEL_VARIANTS[modelIndex_];
	check(model_->loadFromGLTF(QString(":/Models/%1.glb").arg(model_variant.name)));
	model_->setScale(model_variant.scale);
	model_->setPosition(model_variant.position);
}

void Window::switchLightCamera()
{
	if (cameraId_ == DEFAULT)
	{
		camera_ = std::move(cameraBackup_);
		return;
	}
	if (!cameraBackup_) {
		cameraBackup_ = std::move(camera_);
		camera_ = std::make_unique<Camera>(*cameraBackup_);
	}
	switch (cameraId_)
	{
	case DIRECTIONAL_LIGHT:
		{
			QVector3D oldPos = camera_->getPosition();
			camera_->setPosition(oldPos - lighting_->lights_.dirLight.direction);
			camera_->setTarget(oldPos);
		}
		break;
	case POINT_LIGHT:
		camera_->setPosition(lighting_->lights_.pointLight.position);
		break;
	case SPOTLIGHT:
		camera_->setPosition(lighting_->lights_.spotLight.bulb.position);
		camera_->setTarget(lighting_->lights_.spotLight.direction + camera_->getPosition());
		break;
	default:
		break;
	}
}

void Window::syncLightCamera()
{
	switch (cameraId_)
	{
	case DIRECTIONAL_LIGHT:
		lighting_->lights_.dirLight.direction = camera_->getFront();
		break;
	case POINT_LIGHT:
		lighting_->lights_.pointLight.position = camera_->getPosition();
		break;
	case SPOTLIGHT:
		lighting_->lights_.spotLight.bulb.position = camera_->getPosition();
		lighting_->lights_.spotLight.direction = camera_->getFront();
		break;
	default:
		break;
	}
}

void Window::onRender()
{
	const auto guard = captureMetrics();
	processInput();
	syncLightCamera();

	// Clear buffers
	glClearColor(0.2, 0.2, 0.2, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (modelIndexChanged_) {
		modelIndexChanged_ = false;
		reloadModel();
	}

	const auto& camera = *camera_;
	const auto& glContext = *context();
	lighting_->render(camera);
	model_->render(camera, glContext);

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
