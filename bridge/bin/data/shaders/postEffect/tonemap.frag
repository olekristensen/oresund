#version 150

uniform sampler2D image;
//uniform sampler2D dither;
uniform float exposure;
uniform float gamma;
uniform float time;

in vec2 texCoordVarying;

out vec4 fragColor;


// This set suits the coords of of 0-1.0 ranges..
#define MOD3 vec3(443.8975,397.2973, 491.1871)
#define MOD4 vec4(443.8975,397.2973, 491.1871, 470.7827)


float hash12(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float n2rand( vec2 n )
{
    //float t = 1.0;
    float t = time;
    float nrnd0 = hash12( n + 0.07*t );
    float nrnd1 = hash12( n + 0.11*t );
    return (nrnd0+nrnd1) / 2.0;
}

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
    float noise = n2rand(texCoordVarying) / 127.;
    tonemappedColor += noise;
    fragColor=tonemappedColor;

    /*
    //float bayer = texture(dither, gl_FragCoord.xy / 8.).r * (255./64.);  // scaled to [0..1]
    const float rgbByteMax=255.;
    vec4 rgba=rgbByteMax*tonemappedColor;
    vec4 head=floor(rgba);
    vec4 tail=rgba-head;
    tonemappedColor=head+step(bayer,tail);
    fragColor=tonemappedColor/rgbByteMax;
     */
}
