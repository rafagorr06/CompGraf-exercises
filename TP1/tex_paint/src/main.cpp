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

bool show_uv_render = false;

Texture texture; // textura (compartida por ambas ventanas)
Image image; // imagen (para la textura, Image está en RAM, Texture la envía a GPU)
Model model_chookity; // el objeto a pintar, para renderizar en la ventan principal
Model model_aux; // un quad para cubrir la ventana auxiliar y mostrar la textura

Shader shader_main; // shader para el objeto principal (drawMain)
Shader shader_aux; // shader para la ventana auxiliar (drawTexture)
Shader shader_back; // shader para leer las coordenadas UV

// callbacks del mouse y auxiliares para los callbacks
enum class MouseAction { None, ManipulateView, Draw };
MouseAction mouse_action = MouseAction::None; // qué hacer en el callback del motion si el botón del mouse está apretado
double last_x = 0.0;
double last_y = 0.0;
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
	shader_back = Shader("shaders/back"); //definimos el shader back
	
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
		if(show_uv_render){
			drawBack();
		}else{
			drawMain();
		}
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
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
	glDisable(GL_MULTISAMPLE); //se apaga el antialiasing para no alterar los valores U y V
	
	//preparamos el fondo del back, sera negro con el canal alfa en 0
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	
	shader_back.use(); //genera las coordenadas UV
	setMatrixes(main_window, shader_back); //manda al shader las posicion, coordenadas de la camara y el modelo
	
	// dibujamos el modelo en este buffer oculto
	shader_back.setBuffers(model_chookity.buffers);
	model_chookity.buffers.draw();
	
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
			{"pink"  , {0.749f,0.49f,0.498f,1.f}},
			{"yellow", {0.965f,0.729f,0.106f,1.f}},
			{"black" , {0.f,0.f,0.f,1.f}} };
		
		ImGui::Text("Pallete:");
		for (auto &p : pallete) {
			ImGui::SameLine();
			if (ImGui::ColorButton(p.first, p.second))
				color[0] = p.second.x, color[1] = p.second.y, color[2] = p.second.z;
		}
		
		ImGui::Checkbox("Ver render", &show_uv_render);
		
		if (ImGui::Button("Reload Image")) {
			image = Image("models/chookity.png",true);
			texture.update(image);
		}
	});
}


// ===== callbacks de la ventana auxiliar (textura) =====


void pintarPunto(int center_col, int center_fila){
	
	int r = (int)radius; 
	
	//for para recorrer toda la cuadrilla generada
	for (int df = -r; df <= r; ++df) {
		for (int dc = -r; dc <= r; ++dc) {
			
			//coordenadas del texel a evaluar
			int fila_actual = center_fila + df;
			int columna_actual = center_col + dc;
			
			// miramos los límites para no escribir fuera de la ventana
			if (fila_actual >= 0 && fila_actual < image.GetHeight() &&
				columna_actual >= 0 && columna_actual < image.GetWidth()) {
				
				//chequeamos si el téxel está dentro del círculo (pitagoras)
				if ( (df * df) + (dc * dc) <= (radius * radius) ) {
					
					
					float a = color.a/r;
					glm::vec3 color_viejo = image.GetRGB(fila_actual, columna_actual);
					
					//funciona como un balance de color la suma, si el a es muy chiquito el nuevo color,
					//se vera muy poco y predominara el actual, en caso contrario sera el viejo el que 
					//pierde el protagonismo frente al nuevo
					float nuevo_r = (color.r * a) + (color_viejo.r * (1.0f - a));
					float nuevo_g = (color.g * a) + (color_viejo.g * (1.0f - a));
					float nuevo_b = (color.b * a) + (color_viejo.b * (1.0f - a));
					
					image.SetRGB(fila_actual, columna_actual, {nuevo_r, nuevo_g, nuevo_b});
					//image.SetRGB(fila_actual, columna_actual, {color.r, color.g, color.b});
				}
			}
		}
	}
	
}


void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	
	if (action == GLFW_PRESS) {
		if (button != GLFW_MOUSE_BUTTON_LEFT) return;
		
		mouse_action = MouseAction::Draw;
		
		//obtenemos las coordenadas en la ventana
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos); 
		
		//guardamos la posicion actual en las variables globales
		last_x = xpos;
		last_y = ypos;
		
		BufferSize bs = getBufferSize(window); //obtenemos el tamaño de la ventana
		
		//sacamos el "porcentaje" de las coordenadas
		double u = xpos / bs.width; 
		double v = 1 - (ypos / bs.height);
		
		//coordenadas en texeles
		int center_col = (int)(u * image.GetWidth());
		int center_fila = (int)(v * image.GetHeight());
		
		
		//llamamos a la funcion auxiliar para pintar el punto
		pintarPunto(center_col, center_fila);
		
		//actualizamos la textura en GPU
		glfwMakeContextCurrent(window);
		texture.update(image);
		
	} else {
		mouse_action = MouseAction::None;
	}
}

void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse_action!=MouseAction::Draw) return;
	
	//calculamos las distancias entre la coordenada actual y la anterior
	double dx = xpos - last_x;
	double dy = ypos - last_y;
	
	//determinamos la cantidad de pasos (el mayor es el que tomamos)
	double pasos = std::max(std::abs(dx), std::abs(dy));
	
	if (pasos > 0) {
		//calcula cuánto avanzar por cada paso
		double x_inc = dx / pasos;
		double y_inc = dy / pasos;
		
		//nuesto punto actual arranca desde el punto de inicio, luego le vamos sumando los incrementos
		double current_x = last_x;
		double current_y = last_y;
		
		BufferSize bs = getBufferSize(window);
		
		//recoremos paso a paso y vamos incrementando los current
		for (int i = 0; i <= pasos; ++i) {
			
			//tenemos hacer lo mismo que con el punto, debemos escalar el punto, ya que el tamaño
			//de la ventana y de la imagen no son iguales
			
			//sacamos el "porcentaaje" de la ventana donde estamos
			double u = current_x / bs.width;
			double v = 1.0 - (current_y / bs.height);
			
			//guardamos en center la posicion real en la imagen
			int center_col = (int)(u * image.GetWidth());
			int center_fila = (int)(v * image.GetHeight());
			
			pintarPunto(center_col, center_fila);
			
			current_x += x_inc;
			current_y += y_inc; 
		}
		
		//actualizamos
		glfwMakeContextCurrent(window);
		texture.update(image);
	}
	
	//actualizamos las coordenadas para el siguiente frame
	last_x = xpos;
	last_y = ypos;
	
}


// ===== callbacks de la ventana principal (vista 3D) =====

void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	
	if (action == GLFW_PRESS) {
		if (mods != 0 or button == GLFW_MOUSE_BUTTON_RIGHT) {
			mouse_action = MouseAction::ManipulateView;
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
			return;
		}
		
		mouse_action = MouseAction::Draw;
		
		// renderizamos el modelo con el shader de coordenadas en el buffer trasero invisible
		drawBack();
		
		// obtenemos las coordenadas del mouse en la ventana
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		
		// obtenemos el tamaño de la ventana
		BufferSize bs = getBufferSize(window);
		
		// casteamos a entero e invertimos el eje Y 
		int win_x = (int)xpos;
		int win_y = bs.height - 1 - (int)ypos; 
		
		// leemos el color del pixel exacto donde hicimos click desde el buffer trasero
		glReadBuffer(GL_BACK);
		glm::vec4 color_value;
		glReadPixels(win_x, win_y, 1, 1, GL_RGBA, GL_FLOAT, &(color_value[0]));
		
		// si el alpha es mayor a 0, hicimos click sobre el modelo (no en el fondo)
		if (color_value.a > 0.0f) {
			
			// extraemos las coordenadas UV (guardadas en los canales rojo y verde por el shader)
			double u = color_value.r + (color_value.g / 256.0); //parte entera + parte decimal
			double v = color_value.b + (color_value.a / 256.0); //parte entera + parte decimal
			
			// convertimos las UV a coordenadas de texeles reales (invirtiendo la V)
			int center_col = (int)(u * image.GetWidth());
			int center_fila = (int)(v * image.GetHeight());
			
			// llamamos a la funcion auxiliar para pintar el punto en RAM
			pintarPunto(center_col, center_fila);
			
			// actualizamos la textura en GPU
			glfwMakeContextCurrent(window);
			texture.update(image);
		}
		
		// guardamos la posicion actual para el siguiente frame de movimiento
		last_x = xpos;
		last_y = ypos;
		
	} else {
		if (mouse_action==MouseAction::ManipulateView)
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
		mouse_action = MouseAction::None;
	}
}

void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	// si no estamos dibujando, delegamos el movimiento al manipulador de camara
	if (mouse_action != MouseAction::Draw) {
		if (mouse_action == MouseAction::ManipulateView)
			common_callbacks::mouseMoveCallback(window, xpos, ypos);
		return; 
	}
	
	// calculamos las distancias entre la coordenada actual y la anterior
	double dx = xpos - last_x;
	double dy = ypos - last_y;
	
	// determinamos la cantidad de pasos (el mayor es el que tomamos)
	double pasos = std::max(std::abs(dx), std::abs(dy));
	
	if (pasos > 0) {
		//renderizamos el modelo falso una sola vez para leerlo durante todo el trazo
		drawBack(); 
		
		// calcula cuánto avanzar por cada paso en la pantalla
		double x_inc = dx / pasos;
		double y_inc = dy / pasos;
		
		// nuestro punto actual arranca desde el punto de inicio
		double current_x = last_x;
		double current_y = last_y;
		
		BufferSize bs = getBufferSize(window);
		// preparamos a OpenGL para leer del buffer trasero
		glReadBuffer(GL_BACK);
		
		// recorremos paso a paso avanzando sobre los pixeles de la ventana
		for (int i = 0; i <= pasos; ++i) {
			
			// casteamos la posicion de pantalla a entero e invertimos el eje Y
			int win_x = (int)current_x;
			int win_y = bs.height - 1 - (int)current_y;
			
			// validamos no leer fuera de los limites de la ventana
			if (win_x >= 0 && win_x < bs.width && win_y >= 0 && win_y < bs.height) {
				
				// leemos el color (coordenadas UV) del pixel actual del trazo
				glm::vec4 color_value;
				
				glReadPixels(win_x, win_y, 1, 1, GL_RGBA, GL_FLOAT, &(color_value[0]));
				
				// verificamos si el pixel pertenece al modelo
				if (color_value.a > 0.0f) {
					
					// extraemos las UV
					double u = color_value.r + (color_value.g / 256.0);
					double v = color_value.b + (color_value.a / 256.0);
					
					// guardamos en center la posicion real en la imagen
					int center_col = (int)(u * image.GetWidth());
					int center_fila = (int)(v * image.GetHeight());
					
					// pintamos el punto en la RAM
					pintarPunto(center_col, center_fila);
				}
			}
			
			// incrementamos los current para el siguiente pixel de la pantalla
			current_x += x_inc;
			current_y += y_inc;
		}
		
		// actualizamos la textura en GPU una sola vez al terminar el trazo
		glfwMakeContextCurrent(window);
		texture.update(image);
	}
	
	// actualizamos las coordenadas para el siguiente frame
	last_x = xpos;
	last_y = ypos;
	
}
