#include "Callbacks.hpp"

#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include "Window.hpp"
#include "Shaders.hpp"

// window and view
namespace common_callbacks {
	
// mouse state
enum class MouseAction { None, Rotate, Zoom, Pan };
MouseAction mouse_action = MouseAction::None;
glm::vec2 last_mouse_pos;

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	CameraSettings &cam = getCamera(window);
	BufferSize bs = getBufferSize(window);
	glm::vec2 current_mouse_pos(xpos,ypos);
	glm::vec2 delta = current_mouse_pos - last_mouse_pos;
	switch(mouse_action) {
	case MouseAction::Pan:
		cam.view_target.x -= delta.x/bs.width*2.f;
		cam.view_target.y += delta.y/bs.height*2.f;
		cam.view_target.x = std::min(1.f,std::max(-1.f,cam.view_target.x));
		cam.view_target.y = std::min(1.f,std::max(-1.f,cam.view_target.y));
	break;
	case MouseAction::Zoom:
		cam.view_fov += (-delta.x/bs.width+delta.y/bs.height)*50.f;
		cam.view_fov = std::min(90.f,std::max(5.f,cam.view_fov));
	break;
	case MouseAction::Rotate:
		cam.model_angle += 5.f*delta.x/bs.width;
		cam.view_angle += 3.f*delta.y/bs.height;
	break;
	default:
		;
	}
	last_mouse_pos = current_mouse_pos;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (Window::isImGuiEnabled(window) and ImGui::GetIO().WantCaptureMouse) return;
	if (button==GLFW_MOUSE_BUTTON_LEFT) {
		if(action == GLFW_RELEASE) mouse_action = MouseAction::None;
		else {
			if      (mods&GLFW_MOD_SHIFT)   mouse_action = MouseAction::Zoom;
			else if (mods&GLFW_MOD_CONTROL) mouse_action = MouseAction::Pan;
			else if (not mods)              mouse_action = MouseAction::Rotate;
		}
	}
}

void viewResizeCallback(GLFWwindow *window, int w, int h) {
	glfwMakeContextCurrent(window);
	glViewport(0,0,w,h); // win_width = w; win_height = h;
}

render_matrixes_t getMatrixes(Window &window) {
	CameraSettings &cam = window.getCamera();
	BufferSize bs = window.getBufferSize();
	return { glm::rotate( glm::mat4(1.f), cam.view_angle, glm::vec3{1.f,0.f,0.f})
		     * glm::rotate(glm::mat4(1.f), cam.model_angle, glm::vec3{0.f,1.f,0.f}),
	         glm::lookAt( cam.view_pos, cam.view_target, glm::vec3{0.f,1.f,0.f} ),
		     cam.use_perspective
			   ? glm::perspective( glm::radians(cam.view_fov), float(bs.width)/float(bs.height), 0.1f, 100.f )
			   : glm::ortho(-1.5f,+1.5f,-1.5f,+1.5f,-100.f,+100.f)
	       };
}

} // namespace

void setCommonCallbacks(GLFWwindow * window) {
	glfwSetFramebufferSizeCallback(window,common_callbacks::viewResizeCallback);
	glfwSetCursorPosCallback(window, common_callbacks::mouseMoveCallback);
	glfwSetMouseButtonCallback(window, common_callbacks::mouseButtonCallback);
}

void setMatrixes(Window &window, Shader &shader) {
	auto ms = common_callbacks::getMatrixes(window);
	shader.setMatrixes(ms[0],ms[1],ms[2]);
}

void setMatrixes(Window &window, Shader &shader, const glm::mat4 &model_matrix) {
	auto ms = common_callbacks::getMatrixes(window);
	shader.setMatrixes(model_matrix,ms[1]*ms[0],ms[2]);
}

