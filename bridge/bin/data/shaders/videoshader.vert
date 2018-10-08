#version 150

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseProjectionMatrix;
uniform mat4 roomMatrix;
uniform vec2 videoDimensions;
uniform vec2 videoOffset;
uniform vec4 origin;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec2 texCoordVarying;
out vec3 positionVarying;

void main() {
    gl_Position = modelViewProjectionMatrix * position;
    vec4 modelPos = modelViewMatrix * position;
    //vec4 modelPos = roomMatrix * modelViewMatrix position;
    positionVarying = modelPos.xyz;
    vec3 originMV = vec4(modelViewMatrix * origin).xyz;
    texCoordVarying = (vec2(1.0,1.0) - (abs((positionVarying - originMV).xy) / videoDimensions.xy)) + videoOffset ;
}
