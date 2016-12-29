#include "anim.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define ROOT_NODE_ID 255

#define ERR_NO_MEM "out of memory"
#define ERR_OPENGL "OpenGL error"

/**
 * Find the skeleton key poses indices for given timestamp.
 */
static void
find_poses(struct Animation *anim, float time, size_t *r_key0, size_t *r_key1)
{
	size_t i = 0;
	for (; i < anim->pose_count - 1; i++) {
		if (time < anim->timestamps[i + 1]) {
			break;
		}
	}
	*r_key0 = i;
	*r_key1 = i + 1;
}

static void
joint_compute_rotation(struct JointPose *p0, struct JointPose *p1, float time, Mat *r_rm)
{
	/* FIXME: fix interpolation
	Qtr rot;
	qtr_lerp(&p0->rot, &p1->rot, time, &rot);
	*r_rm = mat_from_qtr(&rot);
	*/
	mat_ident(r_rm);
	mat_rotateq(r_rm, &p0->rot);
}

static void
joint_compute_scale(struct JointPose *p0, struct JointPose *p1, float time, Mat *r_sm)
{
	/* FIXME: uncomment after fixing rotation
	Vec scale;
	vec_lerp(&p0->scale, &p1->scale, time, &scale);
	mat_ident(r_sm);
	mat_scalev(r_sm, &scale);
	*/
	mat_ident(r_sm);
	mat_scalev(r_sm, &p0->scale);
}

static void
joint_compute_translation(struct JointPose *p0, struct JointPose *p1, float time, Mat *r_tm)
{
	/* FIXME: uncomment after fixing rotation
	Vec trans;
	vec_lerp(&p0->trans, &p1->trans, time, &trans);
	mat_ident(r_tm);
	mat_translatev(r_tm, &trans);
	*/
	mat_ident(r_tm);
	mat_translatev(r_tm, &p0->trans);
}

/**
 * Compute joint pose transformation.
 *
 * This function computes the joint pose transformation for given timestamp by
 * interpolating between the provided key poses.
 * In order to compute the transformation, the function computes the entire
 * parent chain of transformations up to the root node. The pose transformation
 * for each traversed node will be stored in the provided array and the process
 * keeps track of already computed chains and re-uses them.
 */
static const Mat*
joint_compute_pose(
	struct Animation *anim,
	struct SkeletonPose *sp0,
	struct SkeletonPose *sp1,
	uint8_t joint_id,
	float time,
	Mat *transforms,
	bool *computed
) {
	Mat *t = &transforms[joint_id];

	if (!computed[joint_id]) {
		struct Joint *joint = &anim->skeleton->joints[joint_id];

		// lookup the previous and current joint poses
		struct JointPose *p0 = &sp0->joint_poses[joint_id];
		struct JointPose *p1 = &sp1->joint_poses[joint_id];

		// compute interpolated local joint transform
		Mat tm, rm, sm, tmp;
		mat_ident(t);
		joint_compute_translation(p0, p1, time, &tm);
		joint_compute_rotation(p0, p1, time, &rm);
		joint_compute_scale(p0, p1, time, &sm);
		mat_mul(&tm, &rm, &tmp);
		mat_mul(&tmp, &sm, t);

		// if the joint is not the root, pre-multiply the full parent
		// transformation chain
		if (joint->parent != ROOT_NODE_ID) {
			const Mat *parent_t = joint_compute_pose(
				anim,
				sp0,
				sp1,
				joint->parent, time, transforms,
				computed
			);
			mat_mul(parent_t, t, &tmp);
			*t = tmp;
		}

		computed[joint_id] = true;
	}

	return t;
}

struct AnimationInstance*
animation_instance_new(struct Animation *anim, const char **r_err)
{
	const char *err = NULL;
	struct AnimationInstance *inst = malloc(sizeof(struct AnimationInstance));
	if (!inst) {
		err = ERR_NO_MEM;
		goto error;
	}

	size_t n_joints = anim->skeleton->joint_count;
	inst->joint_transforms = malloc(sizeof(Mat) * n_joints);
	inst->skin_transforms = malloc(sizeof(Mat) * n_joints);
	inst->processed_joints = malloc(sizeof(bool) * n_joints);
	if (inst->joint_transforms == NULL ||
	    inst->skin_transforms == NULL ||
	    inst->processed_joints == NULL) {
		err = ERR_NO_MEM;
		goto error;
	}

	for (size_t i = 0; i < n_joints; i++) {
		mat_ident(inst->joint_transforms + i);
		mat_ident(inst->skin_transforms + i);
	}

	inst->anim = anim;
	inst->time = 0.0f;

	// allocate an OpenGL buffer for animation uniform block
	glGenBuffers(1, &inst->uniform_buffer);
	if (!inst->uniform_buffer) {
		err = ERR_OPENGL;
		goto error;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, inst->uniform_buffer);
	/* TODO: uncomment this
	glBufferData(
		GL_UNIFORM_BUFFER,
		ub_animation->size,
		inst->skin_transforms,
		GL_DYNAMIC_DRAW
	);
	*/
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return inst;

error:
	if (r_err) {
		*r_err = err;
	}
	animation_instance_free(inst);
	return NULL;
}

void
animation_instance_free(struct AnimationInstance *inst)
{
	if (inst) {
		glDeleteBuffers(1, &inst->uniform_buffer);
		free(inst->joint_transforms);
		free(inst->skin_transforms);
		free(inst->processed_joints);
		free(inst);
	}
}

int
animation_instance_play(struct AnimationInstance *inst, float dt)
{
	struct Animation *anim = inst->anim;
	int n_joints = anim->skeleton->joint_count;

	// reset joint processing flags
	memset(inst->processed_joints, 0, sizeof(bool) * n_joints);

	// compute the relative animation time in ticks, which default to 25
	// frames (ticks) per second
	inst->time += dt;
	float speed = anim->speed != 0 ? anim->speed : 25.0f;
	float time_in_ticks = inst->time * speed;
	float local_time = fmod(time_in_ticks, anim->duration);

	// lookup the key poses indices for given timestamp
	size_t key0, key1;
	find_poses(anim, local_time, &key0, &key1);

	// compute the pose time where t = 0 matches pose 0 and t = 1 pose 1
	float t0 = anim->timestamps[key0], t1 = anim->timestamps[key1];
	float pose_time = (local_time - t0) / (t1 - t0);

	// lookup the key poses
	struct SkeletonPose *sp0 = &anim->poses[key0], *sp1 = &anim->poses[key1];

	// for each joint, compute its local transformation matrix;
	// the process is iterative and keeps track of which joints have already
	// their transformations computed, in order to re-use them and skip
	// their processing
	for (int j = 0; j < n_joints; j++) {
		if (!inst->processed_joints[j]) {
			joint_compute_pose(
				anim,                   // animation
				sp0,                    // pose before t
				sp1,                    // pose after t
				j,                      // joint index
				pose_time,              // exact pose time
				inst->joint_transforms, // output transforms array
				inst->processed_joints  // joint processing status array
			);
		}
	}

	/* TODO: uncomment this
	// compute skinning matrices for each joint and store the results into
	// uniform buffer
	glBindBuffer(GL_UNIFORM_BUFFER, inst->uniform_buffer);
	Mat *skin_transforms = glMapBufferRange(
		GL_UNIFORM_BUFFER,
		u_skin_transforms->offset,
		u_skin_transforms->size,
		(
			GL_MAP_WRITE_BIT |
			GL_MAP_INVALIDATE_BUFFER_BIT |
			GL_MAP_UNSYNCHRONIZED_BIT
		)
	);
	if (!skin_transforms || glGetError() != GL_NO_ERROR) {
		err("failed to map animation block uniform buffer");
		return 0;
	}
	Mat tmp;
	for (int j = 0; j < n_joints; j++) {
		mat_mul(
			&inst->joint_transforms[j],
			&anim->skeleton->joints[j].inv_bind_pose,
			&tmp
		);
		mat_transpose(&tmp, &skin_transforms[j]);
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	*/

	return 1;
}
