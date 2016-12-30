#version 330 core

layout(location = 0) in vec3  in_position;
layout(location = 1) in vec3  in_normal;
//layout(location = 2) in vec2  in_uv;

#ifdef ENABLE_SKINNING
layout(location = 3) in ivec4 in_joints;
layout(location = 4) in vec4  in_weights;

layout(shared) uniform AnimationBlock {
	mat4 skin_transforms[100];
};

void apply_anim(inout vec3 pos, inout vec3 normal, ivec4 joints, vec4 weights)
{
	mat4 t = mat4(0);
	bool transformed = false;
	for (int i = 0; i < 4; i++) {
		int joint_id = joints[i];
		if (joint_id == 255)
			break;
		t += skin_transforms[joint_id] * weights[i];
		transformed = true;
	}
	if (transformed) {
		pos = vec3(t * vec4(pos, 1.0));
		normal = vec3(t * vec4(normal, 0.0));
	}
}
#endif

out vec3 position;
out vec3 normal;
//out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#ifdef ENABLE_SHADOW_MAPPING
uniform mat4 light_projection;
out vec4 light_space_position;
#endif

void main()
{
	// local space
	position = in_position;
	normal = in_normal;
	//uv = in_uv;

#ifdef ENABLE_SKINNING
	apply_anim(position, normal, in_joints, in_weights);
#endif

	// model space
	position = (model * vec4(position, 1.0)).xyz;

#ifdef ENABLE_SHADOW_MAPPING
	light_space_position = light_projection * vec4(position, 1.0);
#endif

	// view space
	position = (view * vec4(position, 1.0)).xyz;
	normal = normalize((view * model * vec4(normal, 0.0)).xyz);

	// clip space
	gl_Position = projection * vec4(position, 1.0);
}
