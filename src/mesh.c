// use open source standard library features
#define _XOPEN_SOURCE 700

#include "anim.h"
#include "error.h"
#include "file_utils.h"
#include "mesh.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define MESH_VERSION (VERSION_MINOR << 4 | VERSION_MAJOR)

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
	VERTEX_ATTRIB_POSITION,
	VERTEX_ATTRIB_NORMAL,
	VERTEX_ATTRIB_UV,
	VERTEX_ATTRIB_JOINT_IDS,
	VERTEX_ATTRIB_JOINT_WEIGHTS
};

enum {
	VERTEX_HAS_POSITION  = 1,
	VERTEX_HAS_NORMAL    = 1 << 1,
	VERTEX_HAS_UV        = 1 << 2,
	VERTEX_HAS_JOINTS    = 1 << 3
};


static int
init_gl_objects(struct Mesh *m, void *vdata, void *idata)
{
	int result = 1;

	// create VAO
	glGenVertexArrays(1, &m->vao);
	if (!m->vao || glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}
	glBindVertexArray(m->vao);

	// allocate two GL buffers, one for vertex data, other for indices
	GLuint bufs[2];
	glGenBuffers(2, bufs);
	if (bufs[0] == 0 || bufs[1] == 0 || glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}
	m->vbo = bufs[0];
	m->ibo = bufs[1];

	// initialize vertex data buffer
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		m->vertex_count * m->vertex_size,
		vdata,
		GL_STATIC_DRAW
	);
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}

	// enable coord attribute
	glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION);
	size_t offset = 0;
	glVertexAttribPointer(
		VERTEX_ATTRIB_POSITION,
		3,
		GL_FLOAT,
		GL_FALSE,
		m->vertex_size,
		(void*)(offset)
	);
	offset += 12;

	// enable normal attribute
	if (m->vertex_format & VERTEX_HAS_NORMAL) {
		glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL);
		glVertexAttribPointer(
			VERTEX_ATTRIB_NORMAL,
			3,
			GL_FLOAT,
			GL_FALSE,
			m->vertex_size,
			(void*)(offset)
		);
		offset += 12;
	}

	// enable UV attribute
	if (m->vertex_format & VERTEX_HAS_UV) {
		glEnableVertexAttribArray(VERTEX_ATTRIB_UV);
		glVertexAttribPointer(
			VERTEX_ATTRIB_UV,
			2,
			GL_FLOAT,
			GL_FALSE,
			m->vertex_size,
			(void*)(offset)
		);
		offset += 8;
	}

	// initialize joint ID and weight attributes
	if (m->vertex_format & VERTEX_HAS_JOINTS) {
		glEnableVertexAttribArray(VERTEX_ATTRIB_JOINT_IDS);
		glVertexAttribIPointer(
			VERTEX_ATTRIB_JOINT_IDS,
			4,
			GL_UNSIGNED_BYTE,
			m->vertex_size,
			(void*)(offset)
		);
		offset += 4;

		glEnableVertexAttribArray(VERTEX_ATTRIB_JOINT_WEIGHTS);
		glVertexAttribPointer(
			VERTEX_ATTRIB_JOINT_WEIGHTS,
			4,
			GL_UNSIGNED_BYTE,
			GL_TRUE,
			m->vertex_size,
			(void*)(offset)
		);
		offset += 4;
	}

	// initialize index data buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		m->index_count * 4,
		idata,
		GL_STATIC_DRAW
	);

	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}

cleanup:
	// reset the context
	glBindVertexArray(0);

	return result;

error:
	result = 0;
	goto cleanup;
}


struct Mesh*
mesh_from_file(const char *filename)
{
	// read file contents into a buffer
	char *data = NULL;
	size_t size = 0;
	struct Mesh *mesh = NULL;
	if ((size = file_read(filename, &data)) == 0) {
		errf(ERR_INVALID_MESH, "%s", filename);
		return NULL;
	}

	// create mesh; this could fail and we fall through and propagate the
	// error
	mesh = mesh_from_buffer(data, size);

	// cleanup
	free(data);

	return mesh;
}


struct Mesh*
mesh_from_buffer(const void *data, size_t size)
{
	struct Mesh *m = NULL;
	void *vertex_data = NULL;
	void *index_data = NULL;

	// initialize mesh struct
	if (!(m = malloc(sizeof(struct Mesh)))) {
		err(ERR_NO_MEM);
		goto error;
	}
	memset(m, 0, sizeof(struct Mesh));

	// header sanity check
	if (size < HEADER_SIZE ||
	    get_field(data, VERSION_FIELD) != MESH_VERSION) {
		err(ERR_INVALID_MESH);
		goto error;
	}

	// parse the header and check for vertices and indices
	m->vertex_format = get_field(data, FORMAT_FIELD);
	m->vertex_count = get_field(data, VCOUNT_FIELD);
	m->index_count = get_field(data, ICOUNT_FIELD);
	if (!(m->vertex_format & VERTEX_HAS_POSITION) ||
	    m->vertex_count == 0 ||
	    m->index_count == 0) {
		err(ERR_INVALID_MESH);
		goto error;
	}

	// parse root transformation
	m->transform = get_field(data, TRANSFORM_FIELD);

	// compute vertex entry size based on the attributes specified in format
	// field
	m->vertex_size = POSITION_ATTRIB_SIZE;  // VERTEX_HAS_POSITION
	if (m->vertex_format & VERTEX_HAS_NORMAL) {
		m->vertex_size += NORMAL_ATTRIB_SIZE;
	}
	if (m->vertex_format & VERTEX_HAS_UV) {
		m->vertex_size += UV_ATTRIB_SIZE;
	}
	if (m->vertex_format & VERTEX_HAS_JOINTS) {
		m->vertex_size += JOINT_ATTRIB_SIZE;
	}

	size_t offset = HEADER_SIZE;

	// initialize vertex data buffer
	size_t vsize = m->vertex_count * m->vertex_size;
	if (size < offset + vsize) {
		err(ERR_INVALID_MESH);
		goto error;
	}
	if (!(vertex_data = malloc(vsize))) {
		err(ERR_NO_MEM);
		goto error;
	}
	memcpy(vertex_data, data + offset, vsize);
	offset += vsize;

	// initialize index data buffer
	size_t isize = m->index_count * INDEX_SIZE;
	if (size < offset + isize) {
		err(ERR_INVALID_MESH);
		goto error;
	}
	if (!(index_data = malloc(isize))) {
		err(ERR_NO_MEM);
		goto error;
	}
	memcpy(index_data, data + offset, isize);
	offset += isize;

	// build the skeleton
	if (m->vertex_format & VERTEX_HAS_JOINTS) {
		size_t joint_count = get_field(data, JCOUNT_FIELD);
		size_t jsize = joint_count * JOINT_SIZE;
		if (size < offset + jsize) {
			err(ERR_INVALID_MESH);
			goto error;
		}

		if (!(m->skeleton = malloc(sizeof(struct Skeleton))) ||
		    !(m->skeleton->joints = malloc(sizeof(struct Joint) * joint_count))) {
			err(ERR_NO_MEM);
			goto error;
		}

		m->skeleton->joint_count = joint_count;

		for (size_t j = 0; j < joint_count; j++) {
			uint8_t id = *(uint8_t*)(data + offset);
			assert(id < joint_count);

			m->skeleton->joints[id].parent = *(uint8_t*)(data + offset + 1);
			m->skeleton->joints[id].inv_bind_pose = *(Mat*)(data + offset + 2);
			offset += JOINT_SIZE;
		}
	}

	// initialize animations (if there's a skeleton)
	m->anim_count = get_field(data, ACOUNT_FIELD);
	if (m->skeleton && m->anim_count > 0) {
		m->animations = malloc(sizeof(struct Animation) * m->anim_count);
		if (!m->animations) {
			err(ERR_NO_MEM);
			goto error;
		}

		for (size_t a = 0; a < m->anim_count; a++) {
			struct Animation *anim = &m->animations[a];
			anim->skeleton = m->skeleton;
			anim->duration = *(float*)(data + offset);
			anim->speed = *(float*)(data + offset + 4);
			anim->pose_count = *(uint32_t*)(data + offset + 8);

			offset += ANIM_SIZE;

			// read timestamps
			anim->timestamps = malloc(sizeof(float) * anim->pose_count);
			if (!anim->timestamps) {
				err(ERR_NO_MEM);
				// TODO: free timestamp data
				goto error;
			}
			for (size_t t = 0; t < anim->pose_count; t++) {
				anim->timestamps[t] = *(float*)(data + offset);
				offset += 4;
			}

			// read skeleton poses
			anim->poses = malloc(sizeof(struct SkeletonPose) * anim->pose_count);
			if (!anim->poses) {
				err(ERR_NO_MEM);
				// TODO: free animation data
				goto error;
			}
			for (size_t p = 0; p < anim->pose_count; p++) {
				struct SkeletonPose *sp = anim->poses + p;
				sp->skeleton = m->skeleton;

				sp->joint_poses = malloc(
					sizeof(struct JointPose) * m->skeleton->joint_count
				);
				if (!sp->skeleton || !sp->joint_poses) {
					err(ERR_NO_MEM);
					// TODO: free skeleton pose data
					goto error;
				}

				// read joint poses for given timestamp
				for (size_t j = 0; j < m->skeleton->joint_count; j++) {
					uint8_t id = *(uint8_t*)(data + offset);
					assert(id < m->skeleton->joint_count);

					struct JointPose *jp = &sp->joint_poses[id];

					// translation
					float tx = *(float*)(data + offset + 1);
					float ty = *(float*)(data + offset + 5);
					float tz = *(float*)(data + offset + 9);
					jp->trans = vec(tx, ty, tz, 0);

					// rotation
					float rw = *(float*)(data + offset + 13);
					float rx = *(float*)(data + offset + 17);
					float ry = *(float*)(data + offset + 21);
					float rz = *(float*)(data + offset + 25);
					jp->rot = qtr(rw, rx, ry, rz);

					// scale
					float sx = *(float*)(data + offset + 29);
					float sy = *(float*)(data + offset + 33);
					float sz = *(float*)(data + offset + 37);
					jp->scale = vec(sx, sy, sz, 0);

					offset += POSE_SIZE;
				}
			}
		}
	}

	if (!init_gl_objects(m, vertex_data, index_data)) {
		goto error;
	}

cleanup:
	free(index_data);
	free(vertex_data);
	return m;

error:
	mesh_free(m);
	m = NULL;
	goto cleanup;
}

struct Mesh*
mesh_new(
	float vertices[][3],
	float normals[][3],
	float uvs[][2],
	uint8_t joint_ids[][4],
	uint8_t joint_weights[][4],
	size_t vertex_count,
	uint32_t *indices,
	size_t index_count
) {
	assert(vertices != NULL && vertex_count >= 3);
	assert(indices != NULL && index_count >= 3);

	// determine vertex format and size
	int vertex_format = VERTEX_HAS_POSITION;
	short vertex_size = POSITION_ATTRIB_SIZE;
	if (normals != NULL) {
		vertex_size += NORMAL_ATTRIB_SIZE;
		vertex_format |= VERTEX_HAS_NORMAL;
	}
	if (uvs != NULL) {
		vertex_size += UV_ATTRIB_SIZE;
		vertex_format |= VERTEX_HAS_UV;
	}
	if (joint_ids && joint_weights) {
		vertex_size += JOINT_ATTRIB_SIZE;
		vertex_format |= VERTEX_HAS_JOINTS;
	}

	// tightly pack all data arrays into single one
	char vertex_data[vertex_size * vertex_count];
	for (size_t i = 0; i < vertex_count; i++) {
		void *vertex = vertex_data + i * vertex_size;
		size_t offset = 0;

		memcpy(vertex + offset, vertices[i], POSITION_ATTRIB_SIZE);
		offset += POSITION_ATTRIB_SIZE;

		if (normals) {
			memcpy(vertex + offset, normals[i], NORMAL_ATTRIB_SIZE);
			offset += NORMAL_ATTRIB_SIZE;
		}
		if (uvs) {
			memcpy(vertex + offset, uvs[i], UV_ATTRIB_SIZE);
			offset += UV_ATTRIB_SIZE;
		}
		if (joint_ids && joint_weights) {
			memcpy(vertex + offset, joint_ids[i], 4);
			memcpy(vertex + offset + 4, joint_weights[i], 4);
			offset += JOINT_ATTRIB_SIZE;
		}
	}

	// initialize mesh
	struct Mesh *m = malloc(sizeof(struct Mesh));
	if (!m) {
		err(ERR_NO_MEM);
		goto error;
	}
	memset(m, 0, sizeof(struct Mesh));

	m->vertex_format = vertex_format;
	m->vertex_size = vertex_size;
	m->vertex_count = vertex_count;
	m->index_count = index_count;
	m->vertex_format = vertex_format;
	mat_ident(&m->transform);

	if (!init_gl_objects(m, vertex_data, indices)) {
		goto error;
	}

	return m;

error:
	mesh_free(m);
	return NULL;
}

void
mesh_free(struct Mesh *m)
{
	if (m) {
		// destroy OpenGL objects
		glDeleteVertexArrays(1, &m->vao);
		glDeleteBuffers(1, &m->vbo);
		glDeleteBuffers(1, &m->ibo);

		// free animations
		for (size_t a = 0; a < m->anim_count; a++) {
			struct Animation anim = m->animations[a];
			for (size_t p = 0; p < anim.pose_count; p++) {
				free(anim.poses[p].joint_poses);
			}
			free(anim.poses);
			free(anim.timestamps);
		}
		free(m->animations);

		// free skeleton
		if (m->skeleton) {
			free(m->skeleton->joints);
			free(m->skeleton);
		}

		free(m);
	}
}
