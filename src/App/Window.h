#pragma once

#include "Camera.h"
#include "Lighting.h"
#include "Model.h"
#include "SSAO.h"
#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QGroupBox>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>

#include <array>
#include <functional>
#include <memory>

namespace window_internals
{
	struct model_variant {
		QString name;
		float scale;
		QVector3D position{0.f, 0.f, 0.f};
	};
	inline const std::array MODEL_VARIANTS{
		model_variant{"sponza", 0.01f},
		model_variant{"Duck", 0.01f},
		model_variant{"WaterBottle", 5.f, {0.f, 1.f, 0.f}},
		model_variant{"ScatteringSkull", 5.f},
	};

	enum cameras {
		DIRECTIONAL_LIGHT,
		POINT_LIGHT,
		SPOTLIGHT,
		DEFAULT,
	};

	enum ssao_state {
		DISABLED,
		ENABLED,
		ONLY,
	};
} // namespace window_internals

class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public: // fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;

protected:
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	void keyReleaseEvent(QKeyEvent * event) override;

private:
	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();
	void processInput();

	void reloadModel();
	void switchLightCamera();
	void syncLightCamera();
	void createFBOs(const QSize & size);

	QLayout * createSlider(int min, int max, int defaultValue, auto slot, auto valueFormat);
	QLayout * createIntSlider(int min, int max, int defaultValue, auto slot);
	QLayout * createFloatSlider(float min, float max, float defaultValue, float step, auto slot);
	QWidget * addColorDialog(QString name, auto slot);
	std::unique_ptr<QGroupBox> initSettingsUi();
	QGroupBox * initModelSettingsUi();
	QGroupBox * initLightingSettingsUi();
	QGroupBox * initLightingParamsUi();
	QGroupBox * initSSAOSettingsUi();

signals:
	void updateUI();

private:
	std::shared_ptr<QOpenGLShaderProgram> modelProgram_;
	std::shared_ptr<QOpenGLShaderProgram> ssaoProgram_;
	std::shared_ptr<QOpenGLShaderProgram> lightningProgram_;

	std::unique_ptr<QOpenGLFramebufferObject> gBuffer_;
	std::unique_ptr<QOpenGLFramebufferObject> ssaoBuffer_;

	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Lighting> lighting_;
	std::unique_ptr<SSAO> ssao_;
	std::unique_ptr<Model> model_;
	std::unique_ptr<Camera> cameraBackup_{};

	bool firstMouse_{true};
	QPoint lastMousePos_;
	unsigned int modelIndex_ = 1;
	bool modelIndexChanged_ = false;
	unsigned int cameraId_;
	unsigned int ssaoState_ = window_internals::ssao_state::ENABLED;

	QSet<int> pressedKeys_;
	QElapsedTimer deltaTimer_;

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;

	bool settingsOpen_ = false;
	std::unique_ptr<QGroupBox> settingsUi_;
};
