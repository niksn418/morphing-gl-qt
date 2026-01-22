#include "Window.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedLayout>
#include <QSlider>
#include <QtMath>

using namespace window_internals;

QLayout * Window::createSlider(int min, int max, int defaultValue, auto slot, auto valueFormat)
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

QLayout * Window::createIntSlider(int min, int max, int defaultValue, auto slot)
{
	return createSlider(
		min, max, defaultValue, std::move(slot),
		[](int value) { return QString::number(value); }
	);
}

QLayout * Window::createFloatSlider(float min, float max, float defaultValue, float step, auto slot)
{
	return createSlider(
		static_cast<int>(min / step), static_cast<int>(max / step),
		static_cast<int>(defaultValue / step),
		[slot = std::move(slot), step](int value) { slot(value * step); },
		[step](int value) { return QString::number(value * step); }
	);
}

QWidget * Window::addColorDialog(QString name, auto slot)
{
	auto button = new QPushButton(name);
	connect(button, &QPushButton::clicked, this, [slot = std::move(slot), button](bool) {
		QPalette pal = button->palette();
		auto color = QColorDialog::getColor(pal.color(QPalette::Button));
		if (color.isValid()) {
			pal.setColor(QPalette::Button, color);
			button->setPalette(pal);
			slot(QVector3D(color.redF(), color.greenF(), color.blueF()));
		}
	});
	return button;
}

QGroupBox * Window::initModelSettingsUi()
{
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

	auto fallbackTextureLayout = new QHBoxLayout();
	auto modelUseTexture = new QCheckBox();
	modelUseTexture->setTristate(false);
	modelUseTexture->setChecked(true);
	auto modelColor = addColorDialog("Model Color", [this](QVector3D color) {
		model_->useFallbackTexture(color);
	});
	modelColor->setEnabled(false);
	connect(modelUseTexture, &QCheckBox::stateChanged, this, [this, modelColor](int state) {
		model_->useFallbackTexture(state != Qt::Checked);
		modelColor->setEnabled(state != Qt::Checked);
	});
	fallbackTextureLayout->addWidget(modelUseTexture);
	fallbackTextureLayout->addWidget(modelColor);
	modelSettingsLayout->addRow("Use Model's Texture:", fallbackTextureLayout);

	modelSettings->setLayout(modelSettingsLayout);
	modelSettings->setStyleSheet("font-size: 12pt;");
	modelSettings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	modelSettings->setFixedHeight(modelSettings->sizeHint().height());
	return modelSettings;
}

QGroupBox * Window::initLightingParamsUi()
{
	auto lights = Lighting::defaultLights();
	auto params = new QGroupBox("Parameters");
	auto paramsPages = new QStackedLayout();
	auto pageCombo = new QComboBox();
	auto createParamSLider = [this, &lights](float min, float max, auto accessor) {
		return createFloatSlider(
			min, max, accessor(lights), 0.01f,
			[this, accessor](float value) {
				accessor(lighting_->lights_) = value;
			}
		);
	};
	auto addStrengths = [this, createParamSLider](QFormLayout * params, auto lightAccessor) {
		params->addRow("ambientStrength", createParamSLider(
			0.f, 1.f, [lightAccessor](Lights & lights) -> float& {
				return lightAccessor(lights).ambientStrength;
			})
		);
		params->addRow("diffuseStrength", createParamSLider(
			0.f, 1.f, [lightAccessor](Lights & lights) -> float& {
				return lightAccessor(lights).diffuseStrength;
			})
		);
		params->addRow("specularStrength", createParamSLider(
			0.f, 1.f, [lightAccessor](Lights & lights) -> float& {
				return lightAccessor(lights).diffuseStrength;
			})
		);
	};
	auto addAttenuation = [this, createParamSLider](QFormLayout * params, auto lightAccessor) {
		params->addRow("linear", createParamSLider(
			0.f, 1.f, [lightAccessor](Lights & lights) -> float& {
				return lightAccessor(lights).linear;
			})
		);
		params->addRow("quadratic", createParamSLider(
			0.f, 2.f, [lightAccessor](Lights & lights) -> float& {
				return lightAccessor(lights).quadratic;
			})
		);
	};
	auto createCutOffSlider = [this, &lights](auto cutOffAccessor) {
		auto valueTransform = [](int value) {
			return std::cos(qDegreesToRadians(value / 2.f));
		};
		auto invTransform = [](float value) {
			return std::round(qRadiansToDegrees(std::acos(value)) * 2.f);
		};
		return createSlider(
			0, 180, invTransform(cutOffAccessor(lights)),
			[cutOffAccessor, valueTransform, this](int value) {
				cutOffAccessor(lighting_->lights_) = valueTransform(value);
			},
			[](int value) {
				return QString::number(value / 2.f) + "°";
			}
		);
	};

	auto dirContainerWidget = new QWidget;
	auto dirLightParams = new QFormLayout(dirContainerWidget);
	addStrengths(dirLightParams, [](Lights & lights) -> Light& {
		return lights.dirLight.light;
	});
	paramsPages->addWidget(dirContainerWidget);
	pageCombo->addItem("Directional");

	auto pointContainerWidget = new QWidget;
	auto pointLightParams = new QFormLayout(pointContainerWidget);
	addStrengths(pointLightParams, [](Lights & lights) -> Light& {
		return lights.pointLight.light;
	});
	addAttenuation(pointLightParams, [](Lights & lights) -> PointLight& {
		return lights.pointLight;
	});
	paramsPages->addWidget(pointContainerWidget);
	pageCombo->addItem("Point");

	auto spotContainerWidget = new QWidget;
	auto spotlightParams = new QFormLayout(spotContainerWidget);
	addStrengths(spotlightParams, [](Lights & lights) -> Light& {
		return lights.spotLight.bulb.light;
	});
	addAttenuation(spotlightParams, [](Lights & lights) -> PointLight& {
		return lights.spotLight.bulb;
	});
	spotlightParams->addRow("cutOff", createCutOffSlider(
		[](Lights & lights) -> float& {
			return lights.spotLight.cutOff;
		})
	);
	spotlightParams->addRow("outerCutOff", createCutOffSlider(
		[](Lights & lights) -> float& {
			return lights.spotLight.outerCutOff;
		})
	);
	paramsPages->addWidget(spotContainerWidget);
	pageCombo->addItem("Spotlight");

	auto paramsLayout = new QVBoxLayout();
	paramsLayout->addItem(paramsPages);
	paramsLayout->addWidget(pageCombo);
	connect(pageCombo, qOverload<int>(&QComboBox::currentIndexChanged),
			paramsPages, &QStackedLayout::setCurrentIndex);
	pageCombo->setCurrentIndex(0);
	params->setLayout(paramsLayout);
	return params;
}

QGroupBox * Window::initLightingSettingsUi()
{
	auto lightSettings = new QGroupBox("Lighting Settings:");
	auto lightSettingsLayout = new QHBoxLayout();

	auto cameraOption = new QGroupBox("Move Light");
	auto buttons = new QButtonGroup();
	buttons->addButton(new QRadioButton("None"), DEFAULT);
	buttons->addButton(new QRadioButton("Directional"), DIRECTIONAL_LIGHT);
	buttons->addButton(new QRadioButton("Point"), POINT_LIGHT);
	buttons->addButton(new QRadioButton("Spotlight"), SPOTLIGHT);
	buttons->button(DEFAULT)->setChecked(true);
	auto cameraOptionLayout = new QVBoxLayout();
	for (auto button: buttons->buttons())
		cameraOptionLayout->addWidget(button);
	connect(buttons, &QButtonGroup::idToggled, this, [this](int id, bool enabled) {
		if (!enabled) return;
		cameraId_ = id;
		switchLightCamera();
	});
	cameraOption->setLayout(cameraOptionLayout);
	lightSettingsLayout->addWidget(cameraOption);

	auto colorsSettings = new QGroupBox("Colors");
	auto colorsLayout = new QVBoxLayout();
	colorsLayout->addWidget(addColorDialog("Directional", [this](QVector3D color) {
		lighting_->lights_.dirLight.light.color = color;
	}));
	colorsLayout->addWidget(addColorDialog("Point", [this](QVector3D color) {
		lighting_->lights_.pointLight.light.color = color;
	}));
	colorsLayout->addWidget(addColorDialog("Spotlight", [this](QVector3D color) {
		lighting_->lights_.spotLight.bulb.light.color = color;
	}));
	colorsSettings->setLayout(colorsLayout);
	lightSettingsLayout->addWidget(colorsSettings);

	lightSettingsLayout->addWidget(initLightingParamsUi());

	lightSettings->setLayout(lightSettingsLayout);
	lightSettings->setStyleSheet("font-size: 12pt;");
	lightSettings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	lightSettings->setFixedHeight(lightSettings->sizeHint().height());
	return lightSettings;
}

QGroupBox * Window::initSSAOSettingsUi()
{
	auto settings = new QGroupBox("SSAO Settings:");
	auto settingsLayout = new QHBoxLayout();

	auto buttons = new QButtonGroup();
	buttons->addButton(new QRadioButton("Disabled"), ssao_state::DISABLED);
	buttons->addButton(new QRadioButton("Enabled"), ssao_state::ENABLED);
	buttons->addButton(new QRadioButton("Only"), ssao_state::ONLY);
	buttons->button(ssaoState_)->setChecked(true);
	auto ssaoOptionLayout = new QVBoxLayout();
	for (auto button: buttons->buttons())
		ssaoOptionLayout->addWidget(button);
	connect(buttons, &QButtonGroup::idToggled, this, [this](int id, bool enabled) {
		if (!enabled) return;
		ssaoState_ = id;
	});
	settingsLayout->addLayout(ssaoOptionLayout);

	auto defaultSettings = SSAO::defaultSettings();

	auto tweaksLayout = new QVBoxLayout();
	auto useHemisphere = new QCheckBox("Hemisphere Kernel");
	useHemisphere->setTristate(false);
	useHemisphere->setChecked(defaultSettings.hemisphere);
	connect(useHemisphere, &QCheckBox::stateChanged, this, [this](int state) {
		ssao_->settings.hemisphere = state;
	});
	tweaksLayout->addWidget(useHemisphere);
	auto useBlur = new QCheckBox("Blur");
	useBlur->setTristate(false);
	useBlur->setChecked(true);
	connect(useBlur, &QCheckBox::stateChanged, this, [this](int state) {
		useSSAOBlur_ = state;
	});
	tweaksLayout->addWidget(useBlur);
	auto useSmoothCheck = new QCheckBox("Smooth Range Check");
	useSmoothCheck->setTristate(false);
	useSmoothCheck->setChecked(defaultSettings.smoothCheck);
	connect(useSmoothCheck, &QCheckBox::stateChanged, this, [this](int state) {
		ssao_->settings.smoothCheck = state;
	});
	tweaksLayout->addWidget(useSmoothCheck);
	settingsLayout->addLayout(tweaksLayout);

	auto slidersLayout = new QFormLayout();
	slidersLayout->addRow("Samples", createIntSlider(
		1, MAX_SSAO_SAMPLES, MAX_SSAO_SAMPLES,
		[this](int value) {
			ssao_->setSamplesNum(value);
		}
	));
	slidersLayout->addRow("Sample Radius", createFloatSlider(
		.1f, 2.f, defaultSettings.sampleRadius, 0.05f,
		[this](float value) {
			ssao_->settings.sampleRadius = value;
		}
	));
	slidersLayout->addRow("Kernel Radius", createFloatSlider(
		.1f, 2.f, defaultSettings.kernelRadius, 0.05f,
		[this](float value) {
			ssao_->settings.kernelRadius = value;
		}
	));
	slidersLayout->addRow("Bias", createFloatSlider(
		0.f, 1.f, defaultSettings.bias, 0.005f,
		[this](float value) {
			ssao_->settings.bias = value;
		}
	));
	slidersLayout->addRow("Power", createFloatSlider(
		1.f, 4.f, defaultSettings.power, 0.1f,
		[this](float value) {
			ssao_->settings.power = value;
		}
	));
	settingsLayout->addLayout(slidersLayout);

	settings->setLayout(settingsLayout);
	settings->setStyleSheet("font-size: 12pt;");
	settings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	settings->setFixedHeight(settings->sizeHint().height());
	return settings;
}

std::unique_ptr<QGroupBox> Window::initSettingsUi()
{
	auto settingsUi = std::make_unique<QGroupBox>("Settings", this);
	settingsUi->setAlignment(Qt::AlignHCenter);
	settingsUi->setStyleSheet(
		"QGroupBox {"
			"background-color: rgba(128, 128, 128, 128);"
			"font-weight: bold;"
			"font-size: 20pt;"
		"}"
	);
	settingsUi->setContentsMargins(0, 0, 0, 0);
	settingsUi->setVisible(settingsOpen_);

	auto settingsLayout = new QVBoxLayout();
	settingsLayout->addWidget(initModelSettingsUi());
	settingsLayout->addWidget(initLightingSettingsUi());
	settingsLayout->addWidget(initSSAOSettingsUi());
	settingsLayout->addStretch();
	settingsUi->setLayout(settingsLayout);
	return settingsUi;
}
