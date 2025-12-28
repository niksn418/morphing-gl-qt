#include "Window.h"
#include "utils.h"

#include <QComboBox>
#include <QFormLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>
#include <QSlider>

#include <cmath>

namespace
{
	struct model_variant {
		QString name;
		float scale;
		QVector3D position{0.f, 0.f, 0.f};
	};
	const std::array MODEL_VARIANTS = {
		model_variant{"sponza", 0.01f},
		model_variant{"Duck", 0.01f},
		model_variant{"WaterBottle", 5.f, {0.f, 1.f, 0.f}},
	};
}// namespace

template <typename Func1, typename Func2>
QLayout * Window::createSlider(int min, int max, int defaultValue, Func1 slot, Func2 valueFormat)
{
	auto layout = new QHBoxLayout();
	auto slider = new QSlider(Qt::Horizontal);
	slider->setRange(min, max);
	slider->setValue(defaultValue);
	auto valueLabel = new QLabel(valueFormat(defaultValue));
	layout->addWidget(slider);
	layout->addWidget(valueLabel);

	connect(
		slider, &QSlider::valueChanged, this,
		[=, slot = std::move(slot)](int value) {
			slot(value);
			valueLabel->setText(valueFormat(value));
		}
	);
	return layout;
}

template <typename Func>
QLayout * Window::createIntSlider(int min, int max, int defaultValue, Func slot)
{
	return createSlider(
		min, max, defaultValue, std::move(slot),
		[](int value) { return QString::number(value); }
	);
}

template <typename Func>
QLayout * Window::createFloatSlider(float min, float max, float defaultValue, float step, Func slot)
{
	return createSlider(
		static_cast<int>(min / step), static_cast<int>(max / step),
		static_cast<int>(defaultValue / step),
		[slot = std::move(slot), step](int value) { slot(value * step); },
		[step](int value) { return QString::number(value * step); }
	);
}
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

	auto modelSettings = new QGroupBox("Model Settings:");
	auto modelSettingsLayout = new QFormLayout();
	modelSettingsLayout->addRow("Morphing:", createSlider(
		0, 100, 0, [this](int value) { model_->setMorphing(value / 100.f); },
		[](int value) { return QString::number(value) + '%'; }
	));
	auto modelVariants = new QComboBox();
	for (const auto & model: MODEL_VARIANTS)
		modelVariants->addItem(model.name);
	modelVariants->setCurrentIndex(modelIndex_);
	connect(modelVariants, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		modelIndex_ = index;
		modelIndexChanged_ = true;
	});
	modelSettingsLayout->addRow("Model Name:", modelVariants);
	modelSettings->setLayout(modelSettingsLayout);
	modelSettings->setStyleSheet("font-size: 14pt;");
	modelSettings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	modelSettings->setFixedHeight(modelSettings->sizeHint().height());

	auto settingsLayout = new QVBoxLayout();
	settingsLayout->addWidget(modelSettings);
	settingsLayout->addStretch();
	settingsUi_->setLayout(settingsLayout);

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
	check(model_->loadFromGLTF(QString(":/Models/%1.glb").arg(MODEL_VARIANTS[modelIndex_].name)));
	model_->setScale(MODEL_VARIANTS[modelIndex_].scale);
	model_->setPosition(MODEL_VARIANTS[modelIndex_].position);
}

void Window::onRender()
{
	const auto guard = captureMetrics();
	processInput();

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
