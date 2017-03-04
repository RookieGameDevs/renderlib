#include "error.h"
#include "scene.h"
#include <datalib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Scene {
	struct HashTable *objects;
};

enum {
	OBJECT_TYPE_NULL,
	OBJECT_TYPE_MESH,
	OBJECT_TYPE_TEXT,
	OBJECT_TYPE_QUAD
};

struct ObjectInfo {
	int type;
};

struct MeshInfo {
	struct ObjectInfo base;
	struct Mesh *mesh;
	struct MeshRenderProps *props;
};

struct TextInfo {
	struct ObjectInfo base;
	struct Text *text;
	struct TextRenderProps *props;
};

struct QuadInfo {
	struct ObjectInfo base;
	struct Quad *quad;
	struct QuadRenderProps *props;
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
object_new(void)
{
	struct Object *obj = malloc(sizeof(struct Object));
	if (!obj) {
		return NULL;
	}
	obj->position = vec(0, 0, 0, 0);
	obj->scale = vec(1, 1, 1, 0);
	obj->rotation = qtr(1, 0, 0, 0);
	return obj;
}

struct Object*
scene_add_mesh(struct Scene *scene, struct Mesh *mesh)
{
	struct Object *obj = object_new();
	if (!obj) {
		return NULL;
	}

	struct MeshInfo *info = malloc(sizeof(struct MeshInfo));
	if (!info) {
		free(obj);
		return NULL;
	}

	info->base.type = OBJECT_TYPE_MESH;
	info->mesh = mesh;
	info->props = NULL;  // TODO

	if (!hash_table_set(scene->objects, obj, info)) {
		free(obj);
		free(info);
		return NULL;
	}

	return obj;
}

struct Object*
scene_add_text(struct Scene *scene, struct Text *text)
{
	struct Object *obj = object_new();
	if (!obj) {
		return NULL;
	}

	struct TextInfo *info = malloc(sizeof(struct TextInfo));
	if (!info) {
		free(obj);
		return NULL;
	}

	info->base.type = OBJECT_TYPE_TEXT;
	info->text = text;
	info->props = NULL;  // TODO

	if (!hash_table_set(scene->objects, obj, info)) {
		free(obj);
		free(info);
		return NULL;
	}

	return obj;
}

struct Object*
scene_add_quad(struct Scene *scene, struct Quad *quad)
{
	struct Object *obj = object_new();
	if (!obj) {
		return NULL;
	}

	struct QuadInfo *info = malloc(sizeof(struct QuadInfo));
	if (!info) {
		free(obj);
		return NULL;
	}

	info->base.type = OBJECT_TYPE_QUAD;
	info->quad = quad;
	info->props = NULL;  // TODO

	if (!hash_table_set(scene->objects, obj, info)) {
		free(obj);
		free(info);
		return NULL;
	}

	return obj;
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
scene_render(struct Scene *scene, struct Camera *camera)
{
	const struct Object *obj = NULL;
	struct ObjectInfo *info = NULL;
	struct HashTableIter iter;
	hash_table_iter_init(scene->objects, &iter);
	while (hash_table_iter_next(&iter, (const void**)&obj, (void**)&info)) {
		// TODO
	}
	return 1;
}