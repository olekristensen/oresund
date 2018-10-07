#version 150

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseProjectionMatrix;
uniform mat4 roomMatrix;
uniform vec2 videoDimensions;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec2 texCoordVarying;
out vec3 positionVarying;
out vec3 origin;

void main() {
    gl_Position = modelViewProjectionMatrix * position;
    vec4 modelPos = modelViewMatrix * position;
    //vec4 modelPos = roomMatrix * modelViewMatrix position;
    positionVarying = modelPos.xyz;
    origin = vec4(modelViewMatrix * vec4(0.0,0.,0.,1.0)).xyz;
    origin.x += 0.25;
    texCoordVarying = vec2(1.0,1.0) - (abs((positionVarying - origin).xy) / videoDimensions.xy) ;
}
