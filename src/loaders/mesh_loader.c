// renderlib
#include "anim.h"
#include "buffer.h"
#include "error.h"
#include "geometry.h"
#include "loaders.h"

// OpenGL
#include <GL/glew.h>

// standard library
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define MESH_VERSION (VERSION_MINOR << 4 | VERSION_MAJOR)
#define MESH_MAX_JOINTS 100

#define HEADER_SIZE 78
#define POSITION_ATTRIB_SIZE 12
#define NORMAL_ATTRIB_SIZE 12
#define UV_ATTRIB_SIZE 8
#define JOINT_ATTRIB_SIZE 8
#define INDEX_SIZE 4
#define JOINT_SIZE 66
#define ANIM_SIZE  12
#define POSE_SIZE  41

#define VERSION_FIELD   uint8_t,  0
#define FORMAT_FIELD    uint16_t, 1
#define VCOUNT_FIELD    uint32_t, 3
#define ICOUNT_FIELD    uint32_t, 7
#define JCOUNT_FIELD    uint8_t,  11
#define ACOUNT_FIELD    uint16_t, 12
#define TRANSFORM_FIELD Mat,      14

#define invoke(macro, ...) macro(__VA_ARGS__)
#define cast(data, type, offset) (*(type*)(((char*)data) + offset))
#define get_field(data, field) invoke(cast, data, field)

enum {
	VERTEX_HAS_POSITION  = 1,
	VERTEX_HAS_NORMAL    = 1 << 1,
	VERTEX_HAS_UV        = 1 << 2,
	VERTEX_HAS_JOINTS    = 1 << 3
};

/**
 * Parse .mesh header.
 *
 * Parses header data and returns information, sizes and lengths about mesh
 * data.
 */
static int
parse_header(
	void *data,           // .mesh data
	size_t size,          // size of data
	Mat *r_transform,     // returned root transformation matrix
	int *r_format,        // returned vertex format
	size_t *r_vert_size,  // returned vertex size
	size_t *r_vcount,     // returned vertex count
	size_t *r_vsize,      // returned total vertex data section size
	size_t *r_icount,     // returned index count
	size_t *r_isize,      // returned total index data section size
	size_t *r_jcount,     // returned joint count
	size_t *r_jsize,      // returned total joint data section size
	size_t *r_acount      // returned animations count
) {
	// header sanity check: assert that the size of data buffer has at least
	// room for a full header and the .MESH version is supported
	if (size < HEADER_SIZE || get_field(data, VERSION_FIELD) != MESH_VERSION) {
		return 0;
	}

	*r_transform = get_field(data, TRANSFORM_FIELD);

	// parse the vertex format and retrieve the number of vertices, indices
	// and joints and perform sanity checks
	*r_format = get_field(data, FORMAT_FIELD);
	*r_vcount = get_field(data, VCOUNT_FIELD);
	*r_icount = get_field(data, ICOUNT_FIELD);
	*r_jcount = get_field(data, JCOUNT_FIELD);
	*r_acount = get_field(data, ACOUNT_FIELD);
	if (!(*r_format & VERTEX_HAS_POSITION) ||
	    *r_vcount == 0 ||
	    *r_icount == 0 ||
	    *r_jcount > MESH_MAX_JOINTS) {
		return 0;
	}

	// compute the size of a single vertex entry (a single chunk of vertex
	// attributes) based on the vertex format
	// NOTE: each vertex has at least the position attribute
	*r_vert_size = POSITION_ATTRIB_SIZE;
	if (*r_format & VERTEX_HAS_NORMAL) {
		*r_vert_size += NORMAL_ATTRIB_SIZE;
	}
	if (*r_format & VERTEX_HAS_UV) {
		*r_vert_size += UV_ATTRIB_SIZE;
	}
	if (*r_format & VERTEX_HAS_JOINTS) {
		*r_vert_size += JOINT_ATTRIB_SIZE;
	}

	// compute buffer sizes and check them against total data size
	*r_vsize = *r_vcount * *r_vert_size;
	*r_isize = *r_icount * INDEX_SIZE;
	*r_jsize = *r_jcount * JOINT_SIZE;
	if (size < HEADER_SIZE + *r_vsize + *r_isize + *r_jsize) {
		return 0;
	}

	return 1;
}

/**
 * Parse .mesh geometry data.
 *
 * Initializes a `Geometry` struct and its attributes with data, based on actual
 * format.
 */
static struct Geometry*
parse_geometry(
	void *geom_data,
	int format,
	size_t vert_size,
	size_t vcount,
	size_t vsize,
	size_t icount,
	size_t isize
) {
	// allocate new Geometry object
	struct Geometry *geom = geometry_new();
	if (!geom) {
		return NULL;
	}

	// allocate one buffer for entire data set
	struct Buffer *buf = buffer_new(vsize, geom_data, GL_STATIC_DRAW);
	if (!buf) {
		goto error;
	}

	size_t offset = 0;

	// add position attribute
	int ok = geometry_add_attribute(
		geom,
		buf,
		"position",
		GL_FLOAT,
		3,
		vert_size,
		(void*)offset,
		0
	);
	if (!ok) {
		goto error;
	}
	offset += POSITION_ATTRIB_SIZE;

	// add normal attribute, if present
	if (format & VERTEX_HAS_NORMAL) {
		ok = geometry_add_attribute(
			geom,
			buf,
			"normal",
			GL_FLOAT,
			3,
			vert_size,
			(void*)offset,
			0
		);
		if (!ok) {
			goto error;
		}
		offset += NORMAL_ATTRIB_SIZE;
	}

	// add UV attribute, if present
	if (format & VERTEX_HAS_UV) {
		ok = geometry_add_attribute(
			geom,
			buf,
			"uv",
			GL_FLOAT,
			2,
			vert_size,
			(void*)offset,
			0
		);
		if (!ok) {
			goto error;
		}
		offset += UV_ATTRIB_SIZE;
	}

	// add joints attribute, if present
	if (format & VERTEX_HAS_JOINTS) {
		ok = geometry_add_attribute(
			geom,
			buf,
			"joint_indices",
			GL_UNSIGNED_BYTE,
			4,
			vert_size,
			(void*)offset,
			0
		) && geometry_add_attribute(
			geom,
			buf,
			"joint_weights",
			GL_UNSIGNED_BYTE,
			4,
			vert_size,
			(void*)(offset + 4),
			0
		);
		if (!ok) {
			goto error;
		}
		offset += JOINT_ATTRIB_SIZE;
	}

	buffer_free(buf);

	// set indices
	buf = buffer_new(isize, geom_data + vsize, GL_STATIC_DRAW);
	if (!buf) {
		goto error;
	}
	ok = geometry_set_index(geom, buf, GL_UNSIGNED_INT, icount);
	if (!ok) {
		goto error;
	}

	buffer_free(buf);

	return geom;

error:
	buffer_free(buf);
	geometry_free(geom);
	return NULL;
}

static struct Skeleton*
parse_skeleton(void *skel_data, int jcount)
{
	// allocate skeleton
	struct Skeleton *skel = malloc(sizeof(struct Skeleton));
	if (!skel) {
		err(ERR_NO_MEM);
		return NULL;
	}

	// allocate skeleton joints array
	skel->joints = malloc(sizeof(struct Joint) * jcount);
	if (!skel->joints) {
		free(skel);
		return NULL;
	}
	skel->joint_count = jcount;

	// advance the array of joints and destructure them into skeleton's
	// array
	size_t offset = 0;
	for (uint8_t j = 0; j < jcount; j++) {
		uint8_t id = *(uint8_t*)(skel_data + offset);

		// joint IDs must be consistent with the number of joints
		assert(id < jcount);

		skel->joints[id].parent = *(uint8_t*)(skel_data + offset + 1);
		skel->joints[id].inv_bind_pose = *(Mat*)(skel_data + offset + 2);

		offset += JOINT_SIZE;
	}

	return skel;
}

static struct Animation*
parse_animations(void *anim_data, struct Skeleton *skel, unsigned anim_count)
{
	// allocate animations array
	struct Animation *animations = malloc(sizeof(struct Animation) * anim_count);
	if (!animations) {
		err(ERR_NO_MEM);
		return NULL;
	}
	memset(animations, 0, sizeof(struct Animation) * anim_count);

	// for each animation, fill the `Animation` struct with information
	// destructured from raw data and then create the array of timestamps
	// and corresponding skeleton poses
	size_t offset = 0;
	for (unsigned a = 0; a < anim_count; a++) {
		// initialize animation
		struct Animation *anim = &animations[a];
		anim->skeleton = skel;
		anim->duration = *(float*)(anim_data + offset);
		anim->speed = *(float*)(anim_data + offset + 4);
		anim->pose_count = *(uint32_t*)(anim_data + offset + 8);

		offset += ANIM_SIZE;

		// initialize timestamps
		anim->timestamps = malloc(sizeof(float) * anim->pose_count);
		if (!anim->timestamps) {
			err(ERR_NO_MEM);
			goto error;
		}
		for (unsigned t = 0; t < anim->pose_count; t++) {
			anim->timestamps[t] = *(float*)(anim_data + offset);
			offset += 4;
		}

		// initialize skeleton poses
		anim->poses = malloc(sizeof(struct SkeletonPose) * anim->pose_count);
		if (!anim->poses) {
			err(ERR_NO_MEM);
			goto error;
		}
		memset(anim->poses, 0, sizeof(struct SkeletonPose) * anim->pose_count);
		for (unsigned p = 0; p < anim->pose_count; p++) {
			struct SkeletonPose *sp = anim->poses + p;
			sp->skeleton = skel;
			sp->joint_poses = malloc(sizeof(struct JointPose) * skel->joint_count);
			if (!sp->joint_poses) {
				err(ERR_NO_MEM);
				goto error;
			}

			// read joint poses for given timestamp
			for (size_t j = 0; j < skel->joint_count; j++) {
				uint8_t id = *(uint8_t*)(anim_data + offset);
				assert(id < skel->joint_count);

				struct JointPose *jp = &sp->joint_poses[id];

				// translation
				float tx = *(float*)(anim_data + offset + 1);
				float ty = *(float*)(anim_data + offset + 5);
				float tz = *(float*)(anim_data + offset + 9);
				jp->trans = vec(tx, ty, tz, 0);

				// rotation
				float rw = *(float*)(anim_data + offset + 13);
				float rx = *(float*)(anim_data + offset + 17);
				float ry = *(float*)(anim_data + offset + 21);
				float rz = *(float*)(anim_data + offset + 25);
				jp->rot = qtr(rw, rx, ry, rz);

				// scale
				float sx = *(float*)(anim_data + offset + 29);
				float sy = *(float*)(anim_data + offset + 33);
				float sz = *(float*)(anim_data + offset + 37);
				jp->scale = vec(sx, sy, sz, 0);

				offset += POSE_SIZE;
			}
		}
	}

	return animations;

error:
	for (struct Animation *anim = animations; anim->skeleton; anim++) {
		for (unsigned p = 0; p < anim->pose_count; p++) {
			free(anim->poses[p].joint_poses);
		}
		free(anim->poses);
		free(anim->timestamps);
	}
	free(animations);
	return NULL;
}

int
load_mesh(
	void *data,
	size_t size,
	struct Geometry **r_geometry,
	struct Animation **r_animations,
	unsigned *r_animations_count,
	Mat *r_transform
) {
	assert(data != NULL);
	assert(size > 0);
	assert(r_geometry);

	Mat transform;
	int format = 0;
	size_t vert_size = 0, vcount = 0, icount = 0, jcount = 0, acount = 0;
	size_t vsize = 0, isize = 0, jsize = 0;

	// parse the header and read general information about the mesh
	int ok = parse_header(
		data,
		size,
		&transform,
		&format,
		&vert_size,
		&vcount,
		&vsize,
		&icount,
		&isize,
		&jcount,
		&jsize,
		&acount
	);
	if (!ok) {
		err(ERR_INVALID_MESH);
		return 0;
	}

	// parse mesh geometry
	struct Geometry *geom = parse_geometry(
		data + HEADER_SIZE,
		format,
		vert_size,
		vcount,
		vsize,
		icount,
		isize
	);
	if (!geom) {
		err(ERR_INVALID_MESH);
		return 0;
	}

	// parse the skeleton, if needed and if any
	struct Skeleton *skel = NULL;
	if (r_animations != NULL &&
	    jcount > 0 &&
	    !(skel = parse_skeleton(data + HEADER_SIZE + vsize + isize, jcount))) {
		geometry_free(geom);
		err(ERR_INVALID_MESH);
		return 0;
	}

	// parse animations, if any and a skeleton exists
	struct Animation *animations = NULL;
	if (r_animations != NULL &&
	    acount > 0 &&
	    skel != NULL &&
	    !(animations = parse_animations(data + HEADER_SIZE + vsize + isize + jsize, skel, acount))) {
		free(skel->joints);
		free(skel);
		geometry_free(geom);
		err(ERR_INVALID_MESH);
		return 0;
	}

	*r_geometry = geom;

	if (r_animations) {
		*r_animations = animations;
	}

	if (r_animations_count) {
		*r_animations_count = acount;
	}

	if (r_transform) {
		*r_transform = transform;
	}

	return 1;
}