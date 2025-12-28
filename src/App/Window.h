#pragma once

#include "Camera.h"
#include "Lighting.h"
#include "Model.h"
#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QGroupBox>
#include <QOpenGLShaderProgram>

#include <functional>
#include <memory>

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

	template <typename Func1, typename Func2>
	QLayout * createSlider(int min, int max, int defaultValue, Func1 slot, Func2 valueFormat);
	template <typename Func>
	QLayout * createIntSlider(int min, int max, int defaultValue, Func slot);
	template <typename Func>
	QLayout * createFloatSlider(float min, float max, float defaultValue, float step, Func slot);
	std::unique_ptr<QGroupBox> initSettingsUi();
	QGroupBox * initModelSettingsUi();
	QGroupBox * initLightingSettingsUi();

signals:
	void updateUI();

private:
	std::shared_ptr<QOpenGLShaderProgram> program_;
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Lighting> lighting_;
	std::unique_ptr<Model> model_;
	std::unique_ptr<Camera> cameraBackup_{};

	bool firstMouse_{true};
	QPoint lastMousePos_;
	unsigned int modelIndex_ = 1;
	bool modelIndexChanged_ = false;
	unsigned int cameraId_;

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
