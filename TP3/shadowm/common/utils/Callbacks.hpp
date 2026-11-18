#ifndef CALLBACKS_HPP
#define CALLBACKS_HPP

#include <array>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
class Window;

// window and view
namespace common_callbacks {
	
void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void viewResizeCallback(GLFWwindow *, int w, int h);

struct render_matrixes_t {
	glm::mat4 model, view, projection;
	const glm::mat4 &operator[](int i) const {
		if (i==0) return model;
		if (i==1) return view;
		return projection;
	}
};
render_matrixes_t getMatrixes(Window &window);

} // anonymous namespace

void setCommonCallbacks(GLFWwindow* window);

class Shader;
void setMatrixes(Window &window, Shader &shader);
void setMatrixes(Window &window, Shader &shader, const glm::mat4 &model_matrix);

#endif

