in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;
in vec4 fragPosLightSpace;

uniform sampler2D depthTexture;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 emissionColor;
uniform float shininess;
uniform float opacity;

uniform float ambientStrength;
uniform vec3 lightColor;

out vec4 fragColor;

#include "calcPhong.frag"

uniform float bias; //declaramos la variable de manera global 
uniform int radio; //declaramos la variable de manera global para controlar el radio
uniform bool showFrustumErrors; //decparamos la variable para mostrar o no los errores 

float calcShadow(vec4 fragPosLightSpace) {
	vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	// CASO 1 -> fuera de los límites laterales X/Y de la textura [0, 1]
	if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
		return showFrustumErrors ? -1.0 : 0.0;
	}
	
	// CASO 2 -> fuera del rango de profundidad Near/Far (Z fuera de [0, 1])
	if (projCoords.z < 0.0 || projCoords.z > 1.0) {
		return showFrustumErrors ? -2.0 : 0.0;
	}
	
	float currentDepth = projCoords.z;
	
	float shadow = 0.0;
	int samples = 0;
	//obtenemos el tamaño de 1 texel para poder desplazarnos por los vecinos
	vec2 texelSize = 1.0 / textureSize(depthTexture, 0);

	// recorremos la cuadrícula alrededor del téxel central
	for(int x = -radio; x <= radio; ++x) {
		for(int y = -radio; y <= radio; ++y) {
			float closestDepth = texture(depthTexture, projCoords.xy + vec2(x, y) * texelSize).r;
			// "si mi fragmento está tapado respecto a este téxel vecino específico,
			// sumale 1.0 a shadow; si le da la luz, sumale 0.0"
			shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0; //cant bruta de vecinos con sombra
			samples++; //valdra el tamaño de la cuadrilla
		}
	}
	
	// Retornamos el promedio para lograr el difuminado (valores entre 0.0 y 1.0)
	return shadow / float(samples);
	
}

vec4 calcColor(vec3 textureColor) {
	vec3 phong = calcPhong(lightVSPosition, lightColor, ambientStrength,
						   ambientColor*textureColor, diffuseColor*textureColor,
						   specularColor, shininess);
	
	float shadow = calcShadow(fragPosLightSpace);
	
	// Visualización de los 2 casos de error del frustum de la luz
	if (shadow == -1.0) return vec4(0.1, 0.3, 1.0, 1.0); // Azul: fuera en X o Y (width chico)
	if (shadow == -2.0) return vec4(1.0, 0.1, 0.1, 1.0); // Rojo: fuera en Z (near/far mal ajustados)
	
	vec3 ambientComponent = ambientColor*textureColor*ambientStrength;
	vec3 lighting = mix(phong, ambientComponent, shadow);
	return vec4(lighting+emissionColor,opacity);
}
