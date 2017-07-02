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

struct ObjectRenderContext {
	struct Camera *camera;
	struct Light *light;
	Mat *model;
	Mat *view;
	Mat *projection;
};

struct Object {
	struct ObjectCls *cls;
	Vec position;
	Qtr rotation;
	Vec scale;
	int visible;
};

struct ObjectCls {
	const char *name;

	void
	(*free)(struct Object *obj);

	int
	(*render)(struct Object *obj, struct ObjectRenderContext *ctx);
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
scene_render(
	struct Scene *scene,
	int render_target,
	struct Camera *camera,
	struct Light *light
);

void
scene_free(struct Scene *scene);