#version 150

uniform sampler2D image;

uniform float alpha;
in vec2 texCoordVarying;
in vec3 positionVarying;
out vec4 fragColor;


void main() {
    vec4 videoColor = texture(image, texCoordVarying);
    videoColor.a = alpha;
    fragColor = videoColor;
}
