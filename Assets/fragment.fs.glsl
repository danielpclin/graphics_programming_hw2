#version 410

layout(location = 0) out vec4 fragColor;
layout (location = 1) out vec4 normalColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform sampler2D utex;

void main()
{
	vec3 texColor = texture(utex, vertexData.texcoord).rgb;
	normalColor = vec4(vertexData.N, 1.0);
	fragColor = vec4(texColor, 1.0);
}