#version 330 core
in vec2 fragTexCoords;
out vec4 FragColor;

//recibe las coordenadas de textura original (entre 0.0 y 1.0) y las "divide" para no perder presicion por culpa de los 8bits
vec2 reescalar(float value){
	float a=floor(value*256.0); //toma la parte entera
	float b=fract(value*256.0); //toma la parte decimal
	return vec2(a/256.0,b);
}
	
	void main() {
		vec2 uReesc=reescalar(fragTexCoords.x);
		vec2 vReesc=reescalar(fragTexCoords.y);
		FragColor=vec4(uReesc.x,uReesc.y,vReesc.x,vReesc.y);
	}
	
