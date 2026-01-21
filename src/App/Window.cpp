#include "Window.h"
#include "utils.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <cmath>

using namespace window_internals;

namespace
{
	auto createShader(QObject * owner, const QString & vertexShaderPath,
					  const QString & fragShaderPath)
	{
		auto program = std::make_unique<QOpenGLShaderProgram>(owner);
		check(!!program);
		check(program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath));
		check(program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragShaderPath));
		check(program->link());
		return program;
	}
} // namespace

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
		ssao_.reset();
		ssaoBlur_.reset();
		lighting_.reset();
		camera_.reset();
		modelProgram_.reset();
		ssaoProgram_.reset();
		ssaoBlurProgram_.reset();
		lightningProgram_.reset();
		settingsUi_.reset();
		gBuffer_.reset();
		ssaoBuffer_.reset();
		ssaoBlurBuffer_.reset();
	}
}

void Window::onInit()
{
	// Configure shaders
	modelProgram_ = createShader(this, ":/Shaders/model.vs", ":/Shaders/model.fs");

	camera_ = std::make_unique<Camera>();
	camera_->setPosition(QVector3D(-1.0f, 2.0f, 0.0f));
	camera_->setYaw(0.0f);
	camera_->setPitch(-30.0f);
	camera_->setMoveSpeed(25.0f);
	camera_->setMouseSensitivity(0.1f);

	ssaoProgram_ = createShader(this, ":/Shaders/noop.vs", ":/Shaders/ssao.fs");
	ssao_ = std::make_unique<SSAO>(ssaoProgram_, *context());

	ssaoBlurProgram_ = createShader(this, ":/Shaders/noop.vs", ":/Shaders/blur.fs");
	ssaoBlur_ = std::make_unique<SSAOBlur>(ssaoBlurProgram_);

	lightningProgram_ = createShader(this, ":/Shaders/noop.vs", ":/Shaders/diffuse.fs");
	lighting_ = std::make_unique<Lighting>(lightningProgram_);

	model_ = std::make_unique<Model>(modelProgram_);
	reloadModel();

	createFBOs(rect().size());

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::createFBOs(const QSize & size)
{
	auto changeTexture = [&size, this](GLuint texId, GLint internalformat, GLenum format, GLenum type) {
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, size.width(), size.height(), 0, format, type, NULL);
	};

	gBuffer_.reset();
	gBuffer_ = std::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::Depth);
	gBuffer_->addColorAttachment(size);
	gBuffer_->addColorAttachment(size);
	auto textures = gBuffer_->textures();
	changeTexture(textures[0], GL_RGB16F, GL_RGB, GL_FLOAT);
	changeTexture(textures[1], GL_RGB16F, GL_RGB, GL_FLOAT);
	changeTexture(textures[2], GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	glBindTexture(GL_TEXTURE_2D, 0);
	check(gBuffer_->isValid());
	GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	context()->extraFunctions()->glDrawBuffers(3, bufs);

	ssaoBuffer_.reset();
	ssaoBuffer_ = std::make_unique<QOpenGLFramebufferObject>(size);
	changeTexture(ssaoBuffer_->texture(), GL_RED, GL_RED, GL_FLOAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	check(ssaoBuffer_->isValid());

	ssaoBlurBuffer_.reset();
	ssaoBlurBuffer_ = std::make_unique<QOpenGLFramebufferObject>(size);
	changeTexture(ssaoBlurBuffer_->texture(), GL_RED, GL_RED, GL_FLOAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	check(ssaoBlurBuffer_->isValid());
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
	QVector3D oldPos = camera_->getPosition();
	switch (cameraId_)
	{
	case DIRECTIONAL_LIGHT:
		camera_->setPosition(oldPos - lighting_->lights_.dirLight.direction);
		camera_->setTarget(oldPos);
		break;
	case POINT_LIGHT:
		camera_->setPosition(lighting_->lights_.pointLight.position);
		camera_->setTarget(oldPos);
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

	if (modelIndexChanged_) {
		modelIndexChanged_ = false;
		reloadModel();
	}

	const auto& camera = *camera_;
	const auto& glContext = *context();

	check(gBuffer_->bind());
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model_->render(camera, glContext);
	check(gBuffer_->bindDefault());
	auto textures = gBuffer_->textures();

	std::optional<GLuint> ssaoTexture;
	if (ssaoState_ != ssao_state::DISABLED)
	{
		check(ssaoBuffer_->bind());
		glClear(GL_COLOR_BUFFER_BIT);
		ssao_->render(camera, glContext, textures[0], textures[1]);
		check(ssaoBuffer_->bindDefault());

		ssaoTexture = ssaoBuffer_->texture();

		if (useSSAOBlur_)
		{
			check(ssaoBlurBuffer_->bind());
			glClear(GL_COLOR_BUFFER_BIT);
			ssaoBlur_->render(glContext, ssaoTexture.value());
			check(ssaoBlurBuffer_->bindDefault());

			ssaoTexture = ssaoBlurBuffer_->texture();
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (ssaoState_ != ssao_state::ONLY)
	{
		lighting_->render(camera, glContext,
						textures[0], textures[1], textures[2], ssaoTexture);
	} else {
		QOpenGLFramebufferObject::blitFramebuffer(0, ssaoBuffer_.get());
	}
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

	createFBOs(rect().size());
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
