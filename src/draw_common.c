#include "anim.h"
#include "error.h"
#include "shader.h"
#include <GL/glew.h>

/**
 * Updates uniform buffer with skinning transform data for given animation
 * instance.
 *
 *   inst    Animation instance to retrieve the skin transforms from.
 *   buffer  The name of uniform buffer to be updated.
 *   offset  Skin transforms array offset in the buffer.
 *   size    Skin transforms array size in bytes.
 */
int
update_skin_transforms_buffer(
	struct AnimationInstance *inst,
	GLuint buffer,
	size_t offset,
	size_t size
) {
	// check whether there's enough room for all joint transforms
	if (size < inst->anim->skeleton->joint_count * sizeof(Mat)) {
		errf(
			ERR_NO_MEM,
			"buffer too small for animation skin transforms"
		);
		return 0;
	}

	// bind the buffer to uniform target and map its memory to client
	// address space
	glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	struct Animation *anim = inst->anim;
	Mat *dst = glMapBufferRange(
		GL_UNIFORM_BUFFER,
		offset,
		size,
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
	);
	if (!dst || glGetError() != GL_NO_ERROR) {
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		err(ERR_OPENGL);
		return 0;
	}

	// compute final skinning transforms and store them in the buffer
	Mat tmp;
	for (int j = 0; j < anim->skeleton->joint_count; j++) {
		mat_mul(
			&inst->joint_transforms[j],
			&anim->skeleton->joints[j].inv_bind_pose,
			&tmp
		);
		mat_transpose(&tmp, &dst[j]);
	}

	// unmap the buffer
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return 1;
}

/**
 * Configures skinning-related uniforms.
 *
 *   inst               Animation instance.
 *   shader             Shader program to set the uniforms for.
 *   u_enable_skinning  Skinning toggle flag uniform.
 *   u_skin_transforms  Skin transforms uniform.
 *   ub_aimation        Animation uniform block.
 *   buffer             Uniform buffer which will hold skinning data.
 */
int
configure_skinning(
	struct AnimationInstance *inst,
	struct Shader *shader,
	struct ShaderUniform *u_enable_skinning,
	struct ShaderUniform *u_skin_transforms,
	struct ShaderUniformBlock *ub_animation,
	GLuint buffer
) {
	int enable_skinning = inst != NULL;

	// configure uniforms
	int configured = shader_uniform_set(
		u_enable_skinning,
		1,
		&enable_skinning
	);
	if (!enable_skinning) {
		return configured;
	}

	// update the skin transforms buffer
	int ok = update_skin_transforms_buffer(
		inst,
		buffer,
		u_skin_transforms->offset,
		u_skin_transforms->size
	);
	if (!ok) {
		return 0;
	}

	// perform indexed buffer binding
	GLuint binding_index = 1;
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		binding_index,
		buffer
	);
	glUniformBlockBinding(
		shader->prog,
		ub_animation->index,
		binding_index
	);

	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}
