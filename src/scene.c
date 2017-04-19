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

struct ObjectInfo {
	int type;
	void *ptr;
	void *props;
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
	// update light projection volume
	if (light) {
		light_update_projection(light, camera);
	}

	// setup object-independent transform matrices (view and projection)
	struct Transform transform;
	camera_get_matrices(camera, &transform.view, &transform.projection);
	Mat world_matrix;

	// traverse the scene by iterating the objects' hash table and render
	// each one using related rendering function
	const struct Object *obj = NULL;
	struct ObjectInfo *info = NULL;
	struct HashTableIter iter;
	hash_table_iter_init(scene->objects, &iter);
	while (hash_table_iter_next(&iter, (const void**)&obj, (void**)&info)) {
		// skip invisible objects
		if (!obj->visible) {
			continue;
		}

		// compute object's own global (world) transformation matrix
		mat_ident(&world_matrix);
		mat_translatev(&world_matrix, &obj->position);
		mat_rotateq(&world_matrix, &obj->rotation);
		mat_scalev(&world_matrix, &obj->scale);

		int ok = 1;

		switch (info->type) {
		case OBJECT_TYPE_MESH:
			// apply mesh intrinsic transform to model matrix
			mat_mul(
				&world_matrix,
				&((struct Mesh*)info->ptr)->transform,
				&transform.model
			);
			ok = render_mesh(
				render_target,
				info->ptr,
				info->props,
				&transform,
				light,
				&camera->position
			);
			break;

		case OBJECT_TYPE_TEXT:
			transform.model = world_matrix;
			ok = render_text(
				render_target,
				info->ptr,
				info->props,
				&transform
			);
			break;

		case OBJECT_TYPE_QUAD:
			transform.model = world_matrix;
			ok = render_quad(
				render_target,
				info->ptr,
				info->props,
				&transform
			);
		}
		if (!ok) {
			return 0;
		}
	}
	return 1;
}