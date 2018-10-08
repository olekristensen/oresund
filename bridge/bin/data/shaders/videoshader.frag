#version 150

uniform sampler2D image;

in vec2 texCoordVarying;
in vec3 positionVarying;
out vec4 fragColor;


void main() {
    vec4 videoColor = texture(image, texCoordVarying);
    videoColor.a = 1.0;
    fragColor = videoColor;
}
