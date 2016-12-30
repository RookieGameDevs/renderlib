#version 330 core

in vec3 position;
in vec3 normal;
//in vec2 uv;

#ifdef ENABLE_SHADOW_MAPPING
in vec4 light_space_position;
#endif

#ifdef ENABLE_LIGHTING
struct Light {
	vec3 direction;
	vec3 color;
	float ambient_intensity;
	float diffuse_intensity;
};

uniform Light light;
uniform vec3 eye;

struct Material {
	float specular_intensity;
	float specular_power;
};

uniform Material material;

void apply_lighting(
	inout vec4 color,
	Light light,
	Material mat,
	vec3 eye,
	vec3 position,
	vec3 normal
) {
	// compute ambient color component
	vec4 ambient = vec4(light.color * light.ambient_intensity, 1.0);

	// compute diffuse and specular color components
	float diffuse_factor = dot(normal, -light.direction);
	vec4 diffuse = vec4(0);
	vec4 specular = vec4(0);
	if (diffuse_factor > 0) {
		diffuse = vec4(
			light.color * light.diffuse_intensity * diffuse_factor,
			1.0
		);

		vec3 eye_dir = normalize(eye - position);
		vec3 reflect_dir = normalize(reflect(light.direction, normal));
		float specular_factor = dot(eye_dir, reflect_dir);
		if (specular_factor > 0) {
			specular_factor = pow(specular_factor, mat.specular_power);
			specular = vec4(
				light.color * mat.specular_intensity * specular_factor,
				1.0
			);
		}
	}

	color *= (ambient + diffuse + specular);
}
#endif

#ifdef ENABLE_SHADOW_MAPPING
uniform sampler2D shadow_map;

void apply_shadow(inout vec4 color, sampler2D shadow_map, vec4 position)
{
	vec3 coord = position.xyz / position.w;
	coord = coord * 0.5 + 0.5;
	float bias = 0.005;
	if (coord.z - bias > texture(shadow_map, coord.xy).r) {
		color *= 0.5;
	}
}
#endif

out vec4 color;

void main()
{
	color = vec4(1.0);

#ifdef ENABLE_LIGHTING
	apply_lighting(color, light, material, eye, position, normal);
#endif

#ifdef ENABLE_SHADOW_MAPPING
	apply_shadow(color, shadow_map, light_space_position);
#endif
}