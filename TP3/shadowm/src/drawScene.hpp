#ifndef DRAWSCENE_HPP
#define DRAWSCENE_HPP

#include <glm/glm.hpp>
class Model;
class Window;

// pass: 1=generar shadow map, 2=generar imagen final(aplicar map), 3=normal(sin sombras)
void drawModel(Window &window, const Model &model, const glm::mat4 &m, int pass);
void drawScene(Window &window, int pass);

#endif

