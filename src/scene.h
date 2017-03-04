#pragma once

#include <matlib.h>

struct Mesh;
struct Quad;
struct Text;
struct Camera;

struct Object {
	Vec position;
	Qtr rotation;
	Vec scale;
};

struct Scene;

struct Scene*
scene_new(void);

struct Object*
scene_add_mesh(struct Scene *scene, struct Mesh *mesh);

struct Object*
scene_add_text(struct Scene *scene, struct Text *text);

struct Object*
scene_add_quad(struct Scene *scene, struct Quad *quad);

void
scene_remove_object(struct Scene *scene, struct Object *object);

size_t
scene_object_count(struct Scene *scene);

int
scene_render(struct Scene *scene, struct Camera *camera);

void
scene_free(struct Scene *scene);