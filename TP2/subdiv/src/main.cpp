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
#include "SubDivMesh.hpp"
#include "SubDivMeshRenderer.hpp"

#define VERSION 20251006

// models and settings
std::vector<std::string> models_names = { "cubo", "icosahedron", "plano", "suzanne", "star" };
int current_model = 0;
bool fill = true, nodes = true, wireframe = true, smooth = false, 
	 reload_mesh = true, mesh_modified = false;

// extraa callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);

SubDivMesh mesh;
void subdivide(SubDivMesh &mesh);

int main() {
	
	// initialize window and setup callbacks
	Window window(800, 600, "CG Demo");
	setCommonCallbacks(window);
	glfwSetKeyCallback(window, keyboardCallback);
	CameraSettings &cs = window.getCamera();
	cs.view_fov = 60.f;
	
	// setup OpenGL state and load shaders
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); 
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.8f,0.8f,0.9f,1.f);
	Shader shader_flat("shaders/flat"),
	       shader_smooth("shaders/smooth"),
		   shader_wireframe("shaders/wireframe");
	SubDivMeshRenderer renderer;
	
	// main loop
	Material material;
	material.ka = material.kd = glm::vec3{.8f,.4f,.4f};
	material.ks = glm::vec3{.5f,.5f,.5f};
	material.shininess = 50.f;
	
	FrameTimer timer;
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		if (reload_mesh) {
			mesh = SubDivMesh("models/"+models_names[current_model]+".dat");
			reload_mesh = false; mesh_modified = true;
		}
		if (mesh_modified) {
			renderer = makeRenderer(mesh,false);
			mesh_modified = false;
		}
		
		if (nodes) {
			shader_wireframe.use();
			setMatrixes(window, shader_wireframe);
			renderer.drawPoints(shader_wireframe);
		}
		
		if (wireframe) {
			shader_wireframe.use();
			setMatrixes(window, shader_wireframe);
			renderer.drawLines(shader_wireframe);
		}
		
		if (fill) {
			Shader &shader = smooth ? shader_smooth : shader_flat;
			shader.use();
			setMatrixes(window, shader);
			shader.setLight(glm::vec4{2.f,1.f,5.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.25f);
			shader.setMaterial(material);
			renderer.drawTriangles(shader);
		}
		
		// settings sub-window
		window.ImGuiDialog("CG Example",[&](){
			if (ImGui::Combo(".dat (O)", &current_model,models_names)) reload_mesh = true;
			ImGui::Checkbox("Fill (F)",&fill);
			ImGui::Checkbox("Wireframe (W)",&wireframe);
			ImGui::Checkbox("Nodes (N)",&nodes);
			ImGui::Checkbox("Smooth Shading (S)",&smooth);
			if (ImGui::Button("Subdivide (D)")) { subdivide(mesh); mesh_modified = true; }
			if (ImGui::Button("Reset (R)")) reload_mesh = true;
			ImGui::Text("Nodes: %i, Elements: %i",mesh.n.size(),mesh.e.size());
		});
		
		// finish frame
		window.finishFrame();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
		case 'D': subdivide(mesh); mesh_modified = true; break;
		case 'F': fill = !fill; break;
		case 'N': nodes = !nodes; break;
		case 'W': wireframe = !wireframe; break;
		case 'S': smooth = !smooth; break;
		case 'R': reload_mesh=true; break;
		case 'O': case 'M': current_model = (current_model+1)%models_names.size(); reload_mesh = true; break;
		}
	}
}

// La struct Arista guarda los dos indices de nodos de una arista
// Siempre pone primero el menor indice, para facilitar la búsqueda en lista ordenada;
//    es para usar con el Mapa de más abajo, para asociar un nodo nuevo a una arista vieja
struct Arista {
	int n[2];
	Arista(int n1, int n2) {
		n[0]=n1; n[1]=n2;
		if (n[0]>n[1]) std::swap(n[0],n[1]);
	}
	Arista(Elemento &e, int i) { // i-esima arista de un elemento
		n[0]=e[i]; n[1]=e[i+1];
		if (n[0]>n[1]) std::swap(n[0],n[1]); // pierde el orden del elemento
	}
	const bool operator<(const Arista &a) const {
		return (n[0]<a.n[0]||(n[0]==a.n[0]&&n[1]<a.n[1]));
	}
};

// Mapa sirve para guardar una asociación entre una arista y un indice de nodo (que no es de la arista)
using Mapa = std::map<Arista,int>;

void subdivide(SubDivMesh &mesh) {
	
	int n_orig = mesh.n.size();
	int e_orig = mesh.e.size();
	
	// =========================================================================
	// PASO 1: Por cada elemento original, agregar su centroide
	// Los nuevos nodos quedan en los índices: n_orig a n_orig + e_orig - 1
	// =========================================================================
	for (int ie = 0; ie < e_orig; ++ie) {
		const Elemento &ei = mesh.e[ie];
		glm::vec3 centroide(0.0f);
		for (int j = 0; j < ei.nv; ++j) {
			centroide += mesh.n[ei[j]].p;
		}
		centroide /= static_cast<float>(ei.nv);
		mesh.n.push_back(Nodo(centroide));
	}
	
	// =========================================================================
	// PASO 2: Por cada arista de cada cara, agregar un punto en el medio
	// =========================================================================
	Mapa mapa;
	for (int ie = 0; ie < e_orig; ++ie) {
		Elemento &ei = mesh.e[ie];
		for (int j = 0; j < ei.nv; ++j) {
			Arista arista(ei, j); // Arista entre ei[j] y ei[j+1]
			
			// Evitar procesar dos veces la misma arista interior
			if (mapa.find(arista) != mapa.end()) continue;
			
			glm::vec3 p0 = mesh.n[arista.n[0]].p;
			glm::vec3 p1 = mesh.n[arista.n[1]].p;
			glm::vec3 pto_arista;
			
			int iev = ei.v[j]; // Índice del elemento vecino en la j-ésima arista
			if (iev == -1) {
				// Es arista de frontera: promedio de los 2 vértices de la arista
				pto_arista = (p0 + p1) / 2.0f;
			} else {
				// Es arista interior: promedio de los 2 vértices y los 2 centroides
				glm::vec3 c1 = mesh.n[n_orig + ie].p;
				glm::vec3 c2 = mesh.n[n_orig + iev].p;
				pto_arista = (p0 + p1 + c1 + c2) / 4.0f;
			}
			
			int nuevo_idx = mesh.n.size();
			mesh.n.push_back(Nodo(pto_arista));
			mapa[arista] = nuevo_idx;
		}
	}
	
	// =========================================================================
	// PASO 3: Armar los elementos nuevos (todos serán cuadriláteros)
	// Criterio de orden: [0] = centroide, [1] = arista_ant, [2] = nodo_orig, [3] = arista_sig
	// =========================================================================
	for (int ie = 0; ie < e_orig; ++ie) {
		Elemento ei = mesh.e[ie]; // Copia local porque vamos a reemplazar mesh.e[ie]
		int idx_centroide = n_orig + ie;
		
		for (int j = 0; j < ei.nv; ++j) {
			int idx_arista_ant = mapa[Arista(ei[j - 1], ei[j])];
			int idx_nodo_orig  = ei[j];
			int idx_arista_sig = mapa[Arista(ei[j], ei[j + 1])];
			
			if (j == 0) {
				mesh.reemplazarElemento(ie, idx_centroide, idx_arista_ant, idx_nodo_orig, idx_arista_sig);
			} else {
				mesh.agregarElemento(idx_centroide, idx_arista_ant, idx_nodo_orig, idx_arista_sig);
			}
		}
	}
	
	// Recalcular vecinos y actualizar el atributo es_frontera en la nueva topología
	mesh.makeVecinos();
	
	// =========================================================================
	// PASO 4: Calcular las nuevas posiciones de los nodos originales (0 a n_orig - 1)
	// En cada elemento nuevo del nodo:
	//   elem[0] -> centroide de la cara original (nodo de cara)
	//   elem[1] -> punto medio de arista anterior (arista entre elem[1] y elem[2], vecino v[1])
	//   elem[2] -> el propio nodo original (i)
	//   elem[3] -> punto medio de arista siguiente (arista entre elem[2] y elem[3], vecino v[2])
	// =========================================================================
	for (int i = 0; i < n_orig; ++i) {
		Nodo &ni = mesh.n[i];
		if (ni.e.empty()) continue; // Por si hay algún nodo suelto
		
		glm::vec3 p = ni.p;
		
		if (!ni.es_frontera) {
			// Nodo interior: (4r - f + (n - 3)p) / n
			float cant_elem = static_cast<float>(ni.e.size());
			glm::vec3 f(0.0f);
			glm::vec3 r(0.0f);
			
			for (int ie : ni.e) {
				const Elemento &elem = mesh.e[ie];
				f += mesh.n[elem[0]].p; // Suma el centroide
				// Promedio de los dos puntos de arista de este elemento (como cada arista 
				// se comparte entre 2 elementos del ciclo, sumar (elem[1]+elem[3])/2 
				// en todos los elementos equivale exactamente a sumar una vez cada arista)
				r += (mesh.n[elem[1]].p + mesh.n[elem[3]].p) / 2.0f;
			}
			f /= cant_elem;
			r /= cant_elem;
			
			ni.p = (4.0f * r - f + (cant_elem - 3.0f) * p) / cant_elem;
		} else {
			// Nodo de frontera: (r + p) / 2
			// Donde r es el promedio de los dos puntos medios de las aristas de frontera
			glm::vec3 r(0.0f);
			int cont_frontera = 0;
			
			for (int ie : ni.e) {
				const Elemento &elem = mesh.e[ie];
				// La arista entre elem[1] (pto medio) y elem[2] (nodo original) es la arista 1 del quad
				if (elem.v[1] == -1) {
					r += mesh.n[elem[1]].p;
					cont_frontera++;
				}
				// La arista entre elem[2] (nodo original) y elem[3] (pto medio) es la arista 2 del quad
				if (elem.v[2] == -1) {
					r += mesh.n[elem[3]].p;
					cont_frontera++;
				}
			}
			if (cont_frontera > 0) {
				r /= static_cast<float>(cont_frontera);
				ni.p = (r + p) / 2.0f;
			}
		}
	}
	
	// Esta llamada valida si la estructura de datos quedó consistente
	mesh.verificarIntegridad();
}




//void subdivide(SubDivMesh &mesh) {
	
	/// @@@@@: Implementar Catmull-Clark... lineamientos:
	//  Los nodos originales estan en las posiciones 0 a #n-1 de m.n,
	//  Los elementos orignales estan en las posiciones 0 a #e-1 de m.e
	//  1) Por cada elemento, agregar el centroide (nuevos nodos: #n a #n+#e-1)
	//  2) Por cada arista de cada cara, agregar un pto en el medio que es
	//      promedio de los vertices de la arista y los centroides de las caras 
	//      adyacentes. Aca hay que usar los elementos vecinos.
	//      En los bordes, cuando no hay vecinos, es simplemente el promedio de los 
	//      vertices de la arista
	//      Hay que evitar procesar dos veces la misma arista (como?)
	//      Mas adelante vamos a necesitar determinar cual punto agregamos en cada
	//      arista, y ya que no se pueden relacionar los indices con una formula simple
	//      se sugiere usar Mapa como estructura auxiliar
	//  3) Armar los elementos nuevos
	//      Los quads se dividen en 4, (uno reemplaza al original, los otros 3 se agregan)
	//      Los triangulos se dividen en 3, (uno reemplaza al original, los otros 2 se agregan)
	//      Para encontrar los nodos de las aristas usar el mapa que armaron en el paso 2
	//      Ordenar los nodos de todos los elementos nuevos con un mismo criterio (por ej, 
	//      siempre poner primero al centroide del elemento), para simplificar el paso 4.
	//  4) Calcular las nuevas posiciones de los nodos originales
	//      Para nodos interiores: (4r-f+(n-3)p)/n
	//         f=promedio de nodos interiores de las caras (los agregados en el paso 1)
	//         r=promedio de los pts medios de las aristas (los agregados en el paso 2)
	//         p=posicion del nodo original
	//         n=cantidad de elementos para ese nodo
	//      Para nodos del borde: (r+p)/2
	//         r=promedio de los dos pts medios de las aristas
	//         p=posicion del nodo original
	//      Ojo: en el paso 3 cambio toda la SubDivMesh, analizar donde quedan en los nuevos 
	//      elementos (¿de que tipo son?) los nodos de las caras y los de las aristas 
	//      que se agregaron antes.
	// tips:
	//   no es necesario cambiar ni agregar nada fuera de este método, (con Mapa como 
	//     estructura auxiliar alcanza)
	//   sugerencia: probar primero usando el cubo (es cerrado y solo tiene quads)
	//               despues usando la piramide (tambien cerrada, y solo triangulos)
	//               despues el ejemplo plano (para ver que pasa en los bordes)
	//               finalmente el mono (tiene mezcla y elementos sin vecinos)
	//   repaso de como usar un mapa:
	//     para asociar un indice (i) de nodo a una arista (n1-n2): elmapa[Arista(n1,n2)]=i;
	//     para saber si hay un indice asociado a una arista:  ¿elmapa.find(Arista(n1,n2))!=elmapa.end()?
	//     para recuperar el indice (en j) asociado a una arista: int j=elmapa[Arista(n1,n2)];
	
	
	// Esta llamada valida si la estructura de datos quedó consistente (si todos los
	// índices están dentro del rango válido, y si son correctas las relaciones
	// entre los .n de los elementos y los .e de los nodos). Mantener al final de
	// esta función para ver que la subdivisión implementada no rompa esos invariantes.
//	mesh.verificarIntegridad();
//}
