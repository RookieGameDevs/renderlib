#version 330 core

layout(location = 0) in vec3  position;
layout(location = 1) in vec3  normal;
layout(location = 2) in vec2  uv;
layout(location = 3) in ivec4 joints;
layout(location = 4) in vec4  weights;

out vec3 vert_position;
out vec3 vert_normal;
out vec2 vert_uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool enable_skinning = false;
layout(shared) uniform Animation {
	mat4 skin_transforms[100];
};

void apply_anim(inout vec3 pos, inout vec3 normal, ivec4 joints, vec4 weights)
{
	mat4 t = mat4(0);
	bool transformed = false;
	for (int i = 0; i < 4; i++) {
		int joint_id = joints[i];
		if (joint_id == 255) {
			break;
		}
		t += skin_transforms[joint_id] * weights[i];
		transformed = true;
	}
	if (transformed) {
		pos = vec3(t * vec4(pos, 1.0));
		normal = vec3(t * vec4(normal, 0.0));
	}
}

uniform bool enable_shadow_mapping = false;
uniform mat4 light_space_transform;
out vec4 light_space_position;

void main()
{
	// local space
	vert_position = position;
	vert_normal = normal;
	vert_uv = uv;

	if (enable_skinning) {
		apply_anim(vert_position, vert_normal, joints, weights);
	}

	// model space
	vert_position = (model * vec4(vert_position, 1.0)).xyz;

	if (enable_shadow_mapping) {
		light_space_position = light_space_transform * vec4(vert_position, 1.0);
	}

	// view space
	vert_position = (view * vec4(vert_position, 1.0)).xyz;
	vert_normal = normalize((view * model * vec4(vert_normal, 0.0)).xyz);

	// clip space
	gl_Position = projection * vec4(vert_position, 1.0);
}
