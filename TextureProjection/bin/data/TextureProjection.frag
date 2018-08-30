
#version 150

uniform sampler2DRect projectorTex;

struct MaterialInfo {

	vec3 ka;
	vec3 kd;
	vec3 ks;
	
	float shininess;
};

uniform MaterialInfo material;

struct LightInfo {

	vec3 intensity;
	vec4 position;
};

uniform LightInfo light;

in vec3 eyeNormal;
in vec4 eyePosition;
in vec4 projTextCoord;

out vec4 fragColor;

vec3 phongModel( vec3 pos, vec3 norm ){

	vec3 s = normalize(vec3(light.position)-pos);
	vec3 v = normalize(-pos.xyz);
	vec3 r = reflect( -s, norm);
	
	vec3 ambient = light.intensity * material.ka;
	float sDotN = max( dot(s, norm), 0.0 );
	vec3 diffuse = light.intensity * material.kd * sDotN;
	vec3 spec = vec3(0.0, 0.0, 0.0);
	
	if( sDotN > 0.0 ){
	
		spec = light.intensity * material.ks * pow(max(dot(r,v),0.0), material.shininess);
	}
	
	return ambient + diffuse + spec;
}

void main(){

	vec3 color = phongModel( vec3( eyePosition), eyeNormal);
	vec4 projTexColor = vec4( 0.0, 0.0, 0.0, 0.0);
	
	if( projTextCoord.z > 0.0 ){

		projTexColor = textureProj( projectorTex, projTextCoord );
	}
	
	fragColor = vec4(color, 1.0) + projTexColor;
}
