#version 330 core

layout(location = 0) in vec3  in_position;
layout(location = 3) in ivec4 in_joints;
layout(location = 4) in vec4  in_weights;

uniform mat4 mvp;
uniform bool enable_skinning = false;
layout(shared) uniform Animation {
	mat4 skin_transforms[100];
};

void apply_anim(inout vec3 pos, ivec4 joints, vec4 weights)
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
	}
}

void main()
{
	vec3 position = in_position;
	if (enable_skinning) {
		apply_anim(position, in_joints, in_weights);
	}
	gl_Position = mvp * vec4(position, 1.0);
}
