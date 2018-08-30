#version 150

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseProjectionMatrix;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

in float randomOffset;

out vec2 texCoordVarying;
out vec3 positionVarying;
out vec3 normalVarying;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
	gl_Position = modelViewProjectionMatrix * position;
	positionVarying = position.xyz;
    normalVarying = normal.xyz;
	//randomOffset = rand(position.xy + position.yz);
}
