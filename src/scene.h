#pragma once

#include <matlib.h>

struct Camera;
struct Light;
struct Mesh;
struct Quad;
struct Text;
struct MeshProps;
struct TextProps;
struct QuadProps;

struct Object {
	Vec position;
	Qtr rotation;
	Vec scale;
	Mat transform;
	AABB bounding_box;
	int update;
};

struct Scene;

struct Scene*
scene_new(void);

struct Object*
scene_add_mesh(struct Scene *scene, struct Mesh *mesh, struct MeshProps *props);

struct Object*
scene_add_text(struct Scene *scene, struct Text *text, struct TextProps *props);

struct Object*
scene_add_quad(struct Scene *scene, struct Quad *quad, struct QuadProps *props);

void
scene_remove_object(struct Scene *scene, struct Object *object);

size_t
scene_object_count(struct Scene *scene);

int
scene_render(struct Scene *scene, struct Camera *camera, struct Light *light);

void
scene_free(struct Scene *scene);