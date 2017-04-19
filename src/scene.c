#include "error.h"
#include "renderlib.h"
#include "scene.h"
#include <datalib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Scene {
	struct HashTable *objects;
};

enum {
	OBJECT_TYPE_MESH,
	OBJECT_TYPE_TEXT,
	OBJECT_TYPE_QUAD
};

struct RenderContext {
	struct Camera *camera;
	struct Light *light;
	int render_target;
	Mat view;
	Mat projection;
};

struct ObjectInfo {
	int type;
	void *ptr;
	void *props;
};

typedef int (*RenderFunc)(
	const struct Object*,
	const struct ObjectInfo*,
	const struct RenderContext*
);

inline static Mat
compute_object_matrix(const struct Object *object)
{
	Mat m;
	mat_ident(&m);
	mat_translatev(&m, &object->position);
	mat_rotateq(&m, &object->rotation);
	mat_scalev(&m, &object->scale);
	return m;
}

static int
draw_mesh_object(
	const struct Object *object,
	const struct ObjectInfo *info,
	const struct RenderContext *ctx
) {
	struct Mesh *mesh = info->ptr;
	struct Transform t;
	Mat object_matrix = compute_object_matrix(object);
	mat_mul(&object_matrix, &mesh->transform, &t.model);
	t.view = ctx->view;
	t.projection = ctx->projection;

	return render_mesh(
		ctx->render_target,
		mesh,
		info->props,
		&t,
		ctx->light,
		&ctx->camera->position
	);
}

static int
draw_text_object(
	const struct Object *object,
	const struct ObjectInfo *info,
	const struct RenderContext *ctx
) {
	struct Transform t = {
		.model = compute_object_matrix(object),
		.view = ctx->view,
		.projection = ctx->projection
	};
	return render_text(ctx->render_target, info->ptr, info->props, &t);
}

static int
draw_quad_object(
	const struct Object *object,
	const struct ObjectInfo *info,
	const struct RenderContext *ctx
) {
	struct Transform t = {
		.model = compute_object_matrix(object),
		.view = ctx->view,
		.projection = ctx->projection
	};
	return render_quad(ctx->render_target, info->ptr, info->props, &t);
}

static RenderFunc renderers[] = {
	// OBJECT_TYPE_MESH
	draw_mesh_object,
	// OBJECT_TYPE_TEXT
	draw_text_object,
	// OBJECT_TYPE_QUAD
	draw_quad_object
};

struct Scene*
scene_new(void)
{
	struct Scene *scene = malloc(sizeof(struct Scene));
	if (!scene) {
		err(ERR_NO_MEM);
		return NULL;
	}

	scene->objects = hash_table_new(ptr_hash, ptr_cmp, 0);
	if (!scene->objects) {
		err(ERR_NO_MEM);
		free(scene);
		return NULL;
	}

	return scene;
}

static struct Object*
add_new_object(struct Scene *scene, int type, void *ptr, void *props)
{
	struct Object *obj = malloc(sizeof(struct Object));
	if (!obj) {
		return NULL;
	}
	obj->position = vec(0, 0, 0, 0);
	obj->scale = vec(1, 1, 1, 0);
	obj->rotation = qtr(1, 0, 0, 0);
	obj->visible = 1;

	struct ObjectInfo *info = malloc(sizeof(struct ObjectInfo));
	if (!info) {
		free(obj);
		return NULL;
	}
	info->type = type;
	info->ptr = ptr;
	info->props = props;

	if (!hash_table_set(scene->objects, obj, info)) {
		free(obj);
		free(info);
		return NULL;
	}

	return obj;
}

struct Object*
scene_add_mesh(struct Scene *scene, struct Mesh *mesh, struct MeshProps *props)
{
	return add_new_object(scene, OBJECT_TYPE_MESH, mesh, props);
}

struct Object*
scene_add_text(struct Scene *scene, struct Text *text, struct TextProps *props)
{
	return add_new_object(scene, OBJECT_TYPE_TEXT, text, props);
}

struct Object*
scene_add_quad(struct Scene *scene, struct Quad *quad, struct QuadProps *props)
{
	return add_new_object(scene, OBJECT_TYPE_QUAD, quad, props);
}

void
scene_remove_object(struct Scene *scene, struct Object *object)
{
	struct ObjectInfo *info = hash_table_pop(scene->objects, object);
	free(object);
	free(info);
}

size_t
scene_object_count(struct Scene *scene)
{
	return hash_table_len(scene->objects);
}

void
scene_free(struct Scene *scene)
{
	if (scene) {
		void *k, *v;
		struct HashTableIter iter;
		hash_table_iter_init(scene->objects, &iter);
		while (hash_table_iter_next(&iter, (const void**)&k, &v)) {
			free(k);
			free(v);
		}
		hash_table_free(scene->objects);
		free(scene);
	}
}

int
scene_render(
	struct Scene *scene,
	int render_target,
	struct Camera *camera,
	struct Light *light
) {
	const struct Object *obj = NULL;
	struct ObjectInfo *info = NULL;
	struct RenderContext ctx = {
		.camera = camera,
		.light = light,
		.render_target = render_target,
	};
	camera_get_matrices(camera, &ctx.view, &ctx.projection);

	struct HashTableIter iter;
	hash_table_iter_init(scene->objects, &iter);

	if (light) {
		light_update_projection(light, camera);
	}

	while (hash_table_iter_next(&iter, (const void**)&obj, (void**)&info)) {
		if (!obj->visible) {
			continue;
		}
		if (!renderers[info->type](obj, info, &ctx)) {
			return 0;
		}
	}
	return 1;
}