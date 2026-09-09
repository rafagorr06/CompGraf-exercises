#version 330 core

in vec2 fragTexCoords;
uniform sampler2D colorTexture; // ambient and diffuse components

out vec4 fragColor;

void main() {
	fragColor = texture(colorTexture,fragTexCoords);
}

