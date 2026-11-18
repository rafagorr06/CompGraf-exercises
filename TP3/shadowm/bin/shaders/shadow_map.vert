#version 330 core

in vec3 vertexPosition;

uniform mat4 modelMatrix;

uniform mat4 lightViewMatrix;
uniform mat4 lightProjectionMatrix;


void main() {
	mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
	gl_Position = lightSpaceMatrix * modelMatrix * vec4(vertexPosition,1.f);
}
