#version 330 core

in vec3 position;
in vec3 normal;
in vec2 uv;

out vec4 color;

/*** LIGTHING ***/
uniform bool enable_lighting = false;
uniform vec3 eye;
uniform struct Light {
	vec3 direction;
	vec3 color;
	float ambient_intensity;
	float diffuse_intensity;
} light;
uniform struct Material {
	float specular_intensity;
	float specular_power;
} material;

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

/*** TEXTURE MAPPING ***/
uniform bool enable_texture_mapping = false;
uniform sampler2D texture_map_sampler;

/*** SHADOW MAPPING ***/
uniform bool enable_shadow_mapping = false;
in vec4 light_space_position;
uniform sampler2D shadow_map_sampler;

void apply_shadow(inout vec4 color, sampler2D shadow_map_sampler, vec4 position)
{
	vec3 coord = position.xyz / position.w;
	coord = coord * 0.5 + 0.5;
	float bias = 0.005;
	if (coord.z - bias > texture(shadow_map_sampler, coord.xy).r) {
		color *= 0.5;
	}
}

void main()
{
	color = vec4(1.0);

	if (enable_texture_mapping) {
		color = texture(texture_map_sampler, vec2(uv.x, 1 - uv.y));
	}

	if (enable_lighting) {
		apply_lighting(color, light, material, eye, position, normal);
	}

	if (enable_shadow_mapping) {
		apply_shadow(color, shadow_map_sampler, light_space_position);
	}
}
