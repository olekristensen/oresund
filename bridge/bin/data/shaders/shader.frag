#define PI (3.1415926536)
#define TWO_PI (6.2831853072)

uniform float elapsedTime;
varying vec3 position, normal;
varying float randomOffset;

const vec4 on = vec4(1.);
const vec4 off = vec4(vec3(0.), 1.);

const float waves = 19.;

// triangle wave from 0 to 1
float wrap(float n) {
  return abs(mod(n, 2.)-1.)*-1. + 1.;
}

// creates a cosine wave in the plane at a given angle
float wave(float angle, vec2 point) {
  float cth = cos(angle);
  float sth = sin(angle);
  return (cos (cth*point.x + sth*point.y) + 1.) / 2.;
}

// sum cosine waves at various interfering angles
// wrap values when they exceed 1
float quasi(float interferenceAngle, vec2 point) {
  float sum = 0.;
  for (float i = 0.; i < waves; i++) {
    sum += wave(3.1416*i*interferenceAngle, point);
  }
  return wrap(sum);
}

void main() {
    float stages = 6.;
	float stage = .5; //mod(elapsedTime * .6, stages);

	if(stage == 0.) {
		vec2 normPosition = (position.xz + position.yx) / 100.;
		float b = quasi(elapsedTime*0.002, (normPosition)*200.);
		gl_FragColor = vec4(vec3(b), 1.);
	} else if(stage < 1.) {
		// diagonal stripes
		//if(normal.z == 0.) {
			const float speed = 0.2;
			const float scale = 0.2;
			gl_FragColor = 
				(mod((-position.z + position.y + position.x) + (elapsedTime * speed), scale) < (scale / 2.)) ?
				on : off;
		/*} else {
			gl_FragColor = off;
		}*/
	} else if(stage < 5.) {
		// crazy triangles, grid lines
		float speed = 0.1;
		float scale = 0.2;
		float cutoff = .9;
		vec3 cur = mod(position + speed * elapsedTime, scale) / scale;
		cur *= 1. - abs(normal);
		if(stage < 4.) {
			gl_FragColor = ((cur.x + cur.y + cur.z) < cutoff) ? off : on;
		} else {
			gl_FragColor = (max(max(cur.x, cur.y), cur.z) < cutoff) ? off : on;
		}
	} else if(stage < 6.) {
		// spinning (outline or face) 
		vec2 divider = vec2(cos(elapsedTime), sin(elapsedTime));
		float side = (position.x * divider.y) - (position.y * divider.x);
		gl_FragColor = abs(side) < 0.1 + 0.1 * sin(elapsedTime * 0.) ? on : off;
	}
}
