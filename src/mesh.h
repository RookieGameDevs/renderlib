#pragma once

#include <GL/glew.h>
#include <matlib.h>
#include <stddef.h>

struct Mesh {
	GLuint vao;
	GLuint vbo;
	GLuint ibo;

	int vertex_format;
	size_t vertex_size;
	size_t vertex_count;
	size_t index_count;

	Mat transform;

	struct Skeleton *skeleton;
	struct Animation *animations;
	size_t anim_count;
};

struct Mesh*
mesh_from_file(const char *filename);

struct Mesh*
mesh_from_buffer(const char *data, size_t data_size);

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
);

void
mesh_free(struct Mesh *m);
