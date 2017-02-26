#include "error.h"
#include "scene.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SCENE_CAPACITY_INCREMENT 64

enum {
	OBJECT_TYPE_NULL,
	OBJECT_TYPE_MESH,
	OBJECT_TYPE_TEXT,
	OBJECT_TYPE_QUAD,
};

struct Scene {
	struct Object *objects;
	size_t object_capacity;
	size_t object_count;
};

struct Scene*
scene_new(void)
{
	struct Scene *scene = malloc(sizeof(struct Scene));
	if (!scene) {
		err(ERR_NO_MEM);
		return NULL;
	}

	size_t size = sizeof(struct Object) * SCENE_CAPACITY_INCREMENT;
	scene->objects = malloc(size);
	if (!scene->objects) {
		err(ERR_NO_MEM);
		scene_free(scene);
		return NULL;
	}
	memset(scene->objects, 0, size);
	scene->object_count = 0;
	scene->object_capacity = SCENE_CAPACITY_INCREMENT;

	return scene;
}

/**
 * Allocates a new Object in scene storage.
 */
static struct Object*
scene_alloc_object(struct Scene *scene)
{
	for (size_t i = 0; i < scene->object_capacity; i++) {
		if (scene->objects[i].type == OBJECT_TYPE_NULL) {
			scene->object_count++;
			return &scene->objects[i];
		}
	}

	// allocate a bigger array for objects storage
	size_t cur_size = sizeof(struct Object) * scene->object_capacity;
	size_t new_size = cur_size + sizeof(struct Object) * SCENE_CAPACITY_INCREMENT;
	struct Object *objects = malloc(new_size);
	if (!objects) {
		err(ERR_NO_MEM);
		// if the array could not be allocated, at least the current one
		// is kept
		return NULL;
	}

	// copy existing array and free it
	memcpy(objects, scene->objects, cur_size);
	free(scene->objects);

	// zero the extra space
	memset(((void*)objects) + cur_size, 0, new_size - cur_size);

	// pick the first entry from extra space as object to return and then
	// replace the container
	struct Object *obj = &scene->objects[scene->object_capacity];
	scene->objects = objects;
	scene->object_capacity += SCENE_CAPACITY_INCREMENT;
	scene->object_count++;

	return obj;
}

struct Object*
scene_add_mesh(struct Scene *scene, struct Mesh *mesh)
{
	struct Object *obj = scene_alloc_object(scene);
	obj->type = OBJECT_TYPE_MESH;
	// TODO
	return obj;
}

struct Object*
scene_add_text(struct Scene *scene, struct Text *text)
{
	struct Object *obj = scene_alloc_object(scene);
	obj->type = OBJECT_TYPE_TEXT;
	// TODO
	return obj;
}

struct Object*
scene_add_quad(struct Scene *scene, struct Quad *quad)
{
	struct Object *obj = scene_alloc_object(scene);
	obj->type = OBJECT_TYPE_QUAD;
	// TODO
	return obj;
}

void
scene_remove_object(struct Scene *scene, struct Object *object)
{
	for (size_t i = 0; i < scene->object_capacity; i++) {
		if (&scene->objects[i] == object) {
			scene->objects[i].type = OBJECT_TYPE_NULL;
			scene->object_count--;
			return;
		}
	}
}

size_t
scene_object_count(struct Scene *scene)
{
	return scene->object_count;
}

void
scene_free(struct Scene *scene)
{
	if (scene) {
		free(scene->objects);
		free(scene);
	}
}