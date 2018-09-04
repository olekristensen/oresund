#version 150

uniform sampler2D image;
uniform sampler2D dither;
uniform float exposure;
uniform float gamma;

in vec2 texCoordVarying;

out vec4 fragColor;

// gamma
vec3 toGamma(vec3 v, float gamma) {
    return pow(v, vec3(1.0 / gamma));
}

// tonemap
vec3 tonemapReinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 toneMap(vec3 color, float exposure, float gamma){
    vec3 c = color;
    c *= exposure;
    c = tonemapReinhard(c);
    c = toGamma(c, gamma);
    return c;
}

void main() {
    vec4 color = texture(image, texCoordVarying);
    vec4 tonemappedColor = vec4(toneMap(color.rgb, exposure, gamma), color.a);
    fragColor=tonemappedColor;

    float bayer = texture(dither, gl_FragCoord.xy / 8.).r * (255./64.);  // scaled to [0..1]
    const float rgbByteMax=255.;
    vec4 rgba=rgbByteMax*tonemappedColor;
    vec4 head=floor(rgba);
    vec4 tail=rgba-head;
    tonemappedColor=head+step(bayer,tail);
    fragColor=tonemappedColor/rgbByteMax;
}
