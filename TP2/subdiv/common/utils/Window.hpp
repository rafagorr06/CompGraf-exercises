#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <vector>
#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <imgui.h>

struct CameraSettings {
	glm::vec3 view_target = {0.f,0.f,0.f}, view_pos = {0.f,0.f,3.f};
	float model_angle = 0.f, view_angle = 0.3f, view_fov = 45.f;
	bool use_perspective = true;
};

struct BufferSize { int width=-1, height=-1; };

class Window {
public:
	
	enum Flags { fNone=0, fAntialiasing=2, fBlend=4, fDepth=8, fVSync=16 };
	static int fDefaults; // = fAntialiasing|fDepth|fVSync;
	
	Window() = default;
	Window(int w, int h, const std::string &title, int flags=fDefaults, GLFWwindow *share_context_with=nullptr);
	~Window();
	
	Window(const Window &other) = delete;
	Window &operator=(const Window &other) = delete;
	
	Window(Window &&other);
	Window &operator=(Window &&other);
	
	void ImGuiDialog(const char *title, const std::function<void()> &func);
	ImGuiContext *enableImGui();
	void disableImGui();
	void ImGuiFrame(const std::function<void()> &func);
	operator bool() const { return win_ptr!=nullptr; }
	operator GLFWwindow*() { return win_ptr; }
	operator const GLFWwindow*() const { return win_ptr; }
	
	static bool isImGuiEnabled(GLFWwindow *window);
	void setImGuiScale(float scale);
	
	void bindFrameBuffer(bool and_set_viewport = false);
	
	void finishFrame();
	
	CameraSettings &getCamera() { return m_camera; }
	BufferSize getBufferSize() const;
	
private:
	static int windows_count;
	GLFWwindow *win_ptr = nullptr;
	ImGuiContext *imgui_context = nullptr;
	CameraSettings m_camera;
};

class FrameTimer {
public:
	FrameTimer();
	double newFrame();
	int getFrameRate() const;
private:
	double prev, fps_t;
	int fps = 0, fps_aux=0;
};

Window &getWindow(GLFWwindow* window);
BufferSize getBufferSize(GLFWwindow* window);
CameraSettings &getCamera(GLFWwindow* window);

namespace ImGui {
	bool Combo(const char *label, int *current_item, const std::vector<std::string> &items);
};

#endif
