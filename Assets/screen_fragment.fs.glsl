#version 410
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D colorTexture;
uniform sampler2D blurTexture;
uniform sampler2D normalTexture;
uniform sampler2D weakBlurTexture;

uniform int uMode;
uniform int uTime;
uniform float ubarPos;
uniform vec2 uMouse;
uniform ivec2 uResolution;

const float offset = 1.0 / 200.0;

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);
	
	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}
void main()
{
	vec2 texCoords = TexCoords;
	vec2 screenCoords = TexCoords * uResolution;
	// setup texCoords
	switch(uMode) {
	case 0: // TEXTURE
		break;
	case 1: // NORMAL
		break;
	case 2: // IMAGE_ABSTRACTION
		break;
	case 3: // WATER_COLOR
		texCoords = TexCoords + vec2(noise(TexCoords * 200)/400);
		break;
	case 4: // MAGNIFIER
		if (distance(uMouse * uResolution, TexCoords * uResolution) < 150) {
			texCoords = TexCoords + (uMouse - TexCoords) * 0.5;
		}
		break;
	case 5: // BLOOM
		break;
	case 6: // PIXELIZATION
		texCoords = floor(TexCoords*100.0)/100 + offset;
		break;
	case 7: // SINE_DISTORTION
		texCoords.x = TexCoords.x * 0.9 + (1 + sin(TexCoords.y * radians(360) + radians(uTime * 0.2))) * 0.05;
		break;
	}
	if (uMode != 0 && uMode != 4 && screenCoords.x > uResolution.x * ubarPos) {
		texCoords = TexCoords;
	}

	// sample texture
	vec3 color = texture(colorTexture, texCoords).rgb;
	vec3 normalColor = texture(normalTexture, texCoords).rgb;
	vec3 blurColor = texture(blurTexture, texCoords).rgb;
	vec3 weakBlurColor = texture(weakBlurTexture, texCoords).rgb;
	vec3 fragColor = color;
	
	// setup color
	switch(uMode) {
	case 0: // TEXTURE
		break;
	case 1: // NORMAL
		fragColor = normalColor;
		break;
	case 2: // IMAGE_ABSTRACTION
		vec3 dog = (blurColor - weakBlurColor);
		fragColor = floor(weakBlurColor * 128) / 128 * (1.0 - vec3(clamp((dog.x + dog.y + dog.z) * 5, 0.0, 1.0)));
		break;
	case 3: // WATER_COLOR
		fragColor = floor((color * 0.15 + blurColor * 0.75) * 26) / 26;
		break;
	case 4: // MAGNIFIER
		if (distance(uMouse * uResolution, TexCoords * uResolution) > 150 && distance(uMouse * uResolution, TexCoords * uResolution) < 155) {
			fragColor = vec3(0.0);
		}
		break;
	case 5: // BLOOM
		fragColor = blurColor * 1 + color * 0.2;
		break;
	case 6: // PIXELIZATION
		fragColor = blurColor;
		break;
	case 7: // SINE_DISTORTION
		break;
	}
	if (uMode != 0 && uMode != 4 && screenCoords.x > uResolution.x * ubarPos) {
		fragColor = color;
	}
	if (uMode != 0 && uMode != 4 && distance(uResolution.x * ubarPos, screenCoords.x) < 2) {
		fragColor = vec3(0.8, 0.3, 0.4);
	}
    FragColor = vec4(fragColor, 1.0);
} 