#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"

#define VERSION 20260831

Window main_window; // ventana principal: muestra el modelo en 3d sobre el que se pinta
Window aux_window; // ventana auxiliar que muestra la textura
void drawMain(); // dibuja el modelo "normalmente" para la ventana principal
void drawBack(); // dibuja el modelo con un shader alternativo para convertir coords de la ventana a coords de textura
void drawAux(); // dibuja la textura en la ventana auxiliar
void drawImGui(Window &window); // settings sub-window

float radius = .5; // radio del "pincel" con el que pintamos en la textura
glm::vec4 color = { 0.f, 0.f, 0.f, 1.f }; // color actual con el que se pinta en la textura

Texture texture; // textura (compartida por ambas ventanas)
Image image; // imagen (para la textura, Image está en RAM, Texture la envía a GPU)

Model model_chookity; // el objeto a pintar, para renderizar en la ventan principal
Model model_aux; // un quad para cubrir la ventana auxiliar y mostrar la textura

Shader shader_main; // shader para el objeto principal (drawMain)
Shader shader_aux; // shader para la ventana auxiliar (drawTexture)

// callbacks del mouse y auxiliares para los callbacks
enum class MouseAction { None, ManipulateView, Draw };
MouseAction mouse_action = MouseAction::None; // qué hacer en el callback del motion si el botón del mouse está apretado
void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

int main() {
	
	// main window (3D view)
	
	main_window = Window(800, 600, "Main View");
	glfwSetCursorPosCallback(main_window, mainMouseMoveCallback);
	glfwSetMouseButtonCallback(main_window, mainMouseButtonCallback);
	main_window.getCamera().model_angle = 2.5;
	
	glClearColor(1.f,1.f,1.f,1.f);
	shader_main = Shader("shaders/main");
	
	image = Image("models/chookity.png",true);
	texture = Texture(image);
	
	model_chookity = Model::loadSingle("models/chookity", Model::fNoTextures);
	
	// aux window (texture image)
	Window aux_window(512,512, "Texture", Window::fDefaults, main_window);
	glfwSetCursorPosCallback(aux_window, auxMouseMoveCallback);
	glfwSetMouseButtonCallback(aux_window, auxMouseButtonCallback);
	
	model_aux = Model::loadSingle("models/texquad", Model::fNoTextures);
	shader_aux = Shader("shaders/quad");
	
	// main loop
	do {
		glfwMakeContextCurrent(main_window);
		drawMain();
		drawImGui(main_window);
		glFinish();
		glfwSwapBuffers(main_window);
		
		glfwMakeContextCurrent(aux_window);
		drawAux();
		drawImGui(aux_window);
		glFinish();
		glfwSwapBuffers(aux_window);
		
		glfwPollEvents();
		
	} while( (not glfwWindowShouldClose(main_window)) and (not glfwWindowShouldClose(aux_window)) );
	main_window.disableImGui();
	aux_window.disableImGui();
}


// ===== pasos del renderizado =====

void drawMain() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	shader_main.use();
	setMatrixes(main_window, shader_main);
	shader_main.setLight(glm::vec4{-1.f,1.f,4.f,1.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
	shader_main.setMaterial(model_chookity.material);
	shader_main.setBuffers(model_chookity.buffers);
	model_chookity.buffers.draw();
}

void drawAux() {
	glDisable(GL_DEPTH_TEST);
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	shader_aux.use();
	shader_aux.setMatrixes(glm::mat4{1.f}, glm::mat4{1.f}, glm::mat4{1.f});
	shader_aux.setBuffers(model_aux.buffers);
	model_aux.buffers.draw();
}

void drawBack() {
	glfwMakeContextCurrent(main_window);
	glDisable(GL_MULTISAMPLE);
	
	/// @ToDo: Parte 2: renderizar el modelo en 3d con un nuevo shader de forma 
	///                 que queden las coordenadas de textura de cada fragmento
	///                 en el back-buffer de color
	
	glEnable(GL_MULTISAMPLE);
	glFlush();
	glFinish();
}

void drawImGui(Window &window) {
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
	// settings sub-window
	window.ImGuiDialog("Settings",[&](){
		ImGui::SliderFloat("Radius",&radius,.5,50);
		ImGui::ColorEdit4("Color",&(color[0]),0);
		
		static std::vector<std::pair<const char *, ImVec4>> pallete = { // colores predefindos
		{"white" , {1.f,1.f,1.f,1.f}},
		{"pink", {0.749f,0.49f,0.498f,1.f}},
		{"yellow", {0.965f,0.729f,0.106f,1.f}},
		{"black" , {0.f,0.f,0.f,1.f}} };
		
		ImGui::Text("Pallete:");
		for (auto &p : pallete) {
			ImGui::SameLine();
			if (ImGui::ColorButton(p.first, p.second))
				color[0] = p.second.x, color[1] = p.second.y, color[2] = p.second.z, color[3]=p.second.w;
		}
		
		if (ImGui::Button("Reload Image")) {
			image = Image("models/chookity.png",true);
			texture.update(image);
		}
	});
}


// ===== callbacks de la ventana auxiliar (textura) =====


// COPYRIGHT RAFAEL - ALEJO - 2026
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action == GLFW_PRESS) {
		if (button != GLFW_MOUSE_BUTTON_LEFT) return;
		mouse_action = MouseAction::Draw;
		// 1. Obtener la posición del mouse y mapearla a la textura	
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos); //xpos = 256 = ypos	
		BufferSize bs = getBufferSize(window);
		double u = xpos / bs.width;
		double v = 1 - (ypos / bs.height);
		// Centro del click (en coordenadas de téxeles)
		int center_col = (int)(u * image.GetWidth());
		int center_fila = (int)(v * image.GetHeight());
		// 2. Calcular los límites del Bounding Box según el 'radius'
		int r = (int)radius; 
		// El bucle va desde -r hasta +r alrededor del centro
		for (int df = -r; df <= r; ++df) {
			for (int dc = -r; dc <= r; ++dc) {
				// Coordenadas absolutas del téxel que estamos evaluando
				int current_fila = center_fila + df;
				int current_col = center_col + dc;
				// 3. Chequear límites para no escribir fuera de la memoria
				if (current_fila >= 0 && current_fila < image.GetHeight() &&
					current_col >= 0 && current_col < image.GetWidth()) {
					// 4. Chequear si el téxel está dentro del círculo (Distancia al cuadrado)
					// df^2 + dc^2 <= r^2
					if ( (df * df) + (dc * dc) <= (radius * radius) ) {
						float a = color.a; // El nivel de opacidad del pincel (0.0 a 1.0)
						// Leemos el color que ya está de fondo en la imagen. 
						// Nota: Reemplazá 'GetRGB' por el método que use tu clase Image si se llama distinto.
						glm::vec3 viejo = image.GetRGB(current_fila, current_col);
						// Hacemos la mezcla matemática canal por canal
						float r = (color.r * a) + (viejo.r * (1.0f - a));
						float g = (color.g * a) + (viejo.g * (1.0f - a));
						float b = (color.b * a) + (viejo.b * (1.0f - a));
						image.SetRGB(current_fila, current_col, {r, g, b});
					} 
				}
			}
		}
		// 5. Actualizar la textura en GPU
		glfwMakeContextCurrent(window);
		texture.update(image);
	} else {
		mouse_action = MouseAction::None;
	}
} 



void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse_action!=MouseAction::Draw) return;
	
	/// @ToDo: Parte 1: pintar un segmento de ancho "2*radius" en la imagen
	///                 "image" que se usa como textura
}


// ===== callbacks de la ventana principal (vista 3D) =====

void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action==GLFW_PRESS) {
		if (mods!=0 or button==GLFW_MOUSE_BUTTON_RIGHT) {
			mouse_action = MouseAction::ManipulateView;
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
			return;
		}
		
		mouse_action = MouseAction::Draw;
		
		/// @ToDo: Parte 2: pintar un punto de radio "radius" en la imagen
		///                 "image" que se usa como textura
		
	} else {
		if (mouse_action==MouseAction::ManipulateView)
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
		mouse_action = MouseAction::None;
	}
}

void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse_action!=MouseAction::Draw) {
		if (mouse_action==MouseAction::ManipulateView);
		common_callbacks::mouseMoveCallback(window,xpos,ypos);
		return;
	}
	
	/// @ToDo: Parte 2: pintar un segmento de ancho "2*radius" en la imagen
	///                 "image" que se usa como textura
	
}
