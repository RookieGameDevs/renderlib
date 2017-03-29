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

typedef int (*RenderFunc)(
	struct Object*,
	struct ObjectInfo*,
	struct Camera*,
	struct Light*
);

static void
update_object(struct Object *object, struct ObjectInfo *info)
{
	// compute object world transform matrix
	if (object->update) {
		mat_ident(&object->transform);
		mat_translatev(&object->transform, &object->position);
		mat_rotateq(&object->transform, &object->rotation);
		mat_scalev(&object->transform, &object->scale);
	}

	// update the axis aligned bounding box
	switch (info->type) {
	case OBJECT_TYPE_MESH:
		object->bounding_box = ((struct Mesh*)info->ptr)->bounding_box;
		break;
	case OBJECT_TYPE_TEXT:
		object->bounding_box.near = vec(0, 0, 0, 0);
		object->bounding_box.far = vec(
			((struct Text*)info->ptr)->width,
			((struct Text*)info->ptr)->height,
			0,
			0
		);
		break;
	case OBJECT_TYPE_QUAD:
		object->bounding_box.near = vec(0, 0, 0, 0);
		object->bounding_box.far = vec(
			((struct Quad*)info->ptr)->width,
			((struct Quad*)info->ptr)->height,
			0,
			0
		);
	}
	aabb_transform(&object->bounding_box, &object->transform);
}

static int
draw_mesh_object(
	struct Object *object,
	struct ObjectInfo *info,
	struct Camera *camera,
	struct Light *light
) {
	struct Mesh *mesh = info->ptr;
	struct Transform t;
	mat_mul(&object->transform, &mesh->transform, &t.model);
	camera_get_matrices(camera, &t.view, &t.projection);

	return render_mesh(mesh, info->props, &t, light, &camera->position);
}

static int
draw_text_object(
	struct Object *object,
	struct ObjectInfo *info,
	struct Camera *camera,
	struct Light *light
) {
	struct Transform t;
	t.model = object->transform;
	camera_get_matrices(camera, &t.view, &t.projection);
	return render_text(info->ptr, info->props, &t);
}

static int
draw_quad_object(
	struct Object *object,
	struct ObjectInfo *info,
	struct Camera *camera,
	struct Light *light
) {
	struct Transform t;
	t.model = object->transform;
	camera_get_matrices(camera, &t.view, &t.projection);
	return render_quad(info->ptr, info->props, &t);
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
	obj->update = 1;

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
scene_render(struct Scene *scene, struct Camera *camera, struct Light *light)
{
	struct Object *obj = NULL;
	struct ObjectInfo *info = NULL;
	struct HashTableIter iter;

	// update scene objects
	hash_table_iter_init(scene->objects, &iter);
	while (hash_table_iter_next(&iter, (const void**)&obj, (void**)&info)) {
		update_object(obj, info);
	}

	if (light) {
		// compute scene bounding box
		AABB bblist[scene_object_count(scene)];
		hash_table_iter_init(scene->objects, &iter);
		size_t i = 0;
		while (hash_table_iter_next(&iter, (const void**)&obj, NULL)) {
			bblist[i++] = obj->bounding_box;
		}
		AABB scene_bounding_box = aabb_container(bblist, i);

		// update light projection frustum
		light_update_projection(light, camera, &scene_bounding_box);
	}

	// render scene objects
	hash_table_iter_init(scene->objects, &iter);
	while (hash_table_iter_next(&iter, (const void**)&obj, (void**)&info)) {
		if (!renderers[info->type](obj, info, camera, light)) {
			return 0;
		}
	}
	return 1;
}