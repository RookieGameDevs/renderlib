#include "renderlib.h"
#include <assert.h>
#include <stdlib.h>

static const char *vertex_shader = (
# include "mesh.vert.h"
);

static const char *fragment_shader = (
# include "mesh.frag.h"
);

// defined in draw_common.c
int
update_skin_transforms_buffer(
	struct AnimationInstance *inst,
	GLuint buffer,
	size_t offset,
	size_t size
);

int
configure_skinning(
	struct AnimationInstance *inst,
	struct Shader *shader,
	struct ShaderUniform *u_enable_skinning,
	struct ShaderUniform *u_skin_transforms,
	struct ShaderUniformBlock *ub_animation,
	GLuint buffer
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };
static struct ShaderUniform u_model;
static struct ShaderUniform u_view;
static struct ShaderUniform u_projection;
static struct ShaderUniform u_enable_skinning;
static struct ShaderUniformBlock ub_animation;
static struct ShaderUniform u_skin_transforms;
static struct ShaderUniform u_enable_texture_mapping;
static struct ShaderUniform u_texture_map_sampler;
static struct ShaderUniform u_enable_shadow_mapping;
static struct ShaderUniform u_shadow_map_sampler;
static struct ShaderUniform u_light_space_transform;
static struct ShaderUniform u_enable_lighting;
static struct ShaderUniform u_eye;
static struct ShaderUniform u_light_direction;
static struct ShaderUniform u_light_color;
static struct ShaderUniform u_light_ambient_intensity;
static struct ShaderUniform u_light_diffuse_intensity;
static struct ShaderUniform u_material_color;
static struct ShaderUniform u_material_specular_intensity;
static struct ShaderUniform u_material_specular_power;

static GLuint skin_transforms_buffer = 0;

static void
cleanup(void)
{
	shader_free(shader);
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
	glDeleteBuffers(1, &skin_transforms_buffer);
}

int
init_mesh_pipeline(void)
{
	// cleanup resources at program exit
	atexit(cleanup);

	// uniform names and receiver pointers
	const char *uniform_names[] = {
		"model",
		"view",
		"projection",
		"enable_skinning",
		"enable_texture_mapping",
		"texture_map_sampler",
		"enable_shadow_mapping",
		"shadow_map_sampler",
		"light_space_transform",
		"enable_lighting",
		"eye",
		"light.direction",
		"light.color",
		"light.ambient_intensity",
		"light.diffuse_intensity",
		"material.color",
		"material.specular_intensity",
		"material.specular_power",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_model,
		&u_view,
		&u_projection,
		&u_enable_skinning,
		&u_enable_texture_mapping,
		&u_texture_map_sampler,
		&u_enable_shadow_mapping,
		&u_shadow_map_sampler,
		&u_light_space_transform,
		&u_enable_lighting,
		&u_eye,
		&u_light_direction,
		&u_light_color,
		&u_light_ambient_intensity,
		&u_light_diffuse_intensity,
		&u_material_color,
		&u_material_specular_intensity,
		&u_material_specular_power
	};

	// uniform block names and receiver pointers
	const char *uniform_block_names[] = {
		"Animation",
		NULL
	};
	struct ShaderUniformBlock *uniform_blocks[] = {
		&ub_animation
	};

	// compile mesh pipeline shader and initialize uniforms
	shader_sources[0] = shader_source_from_string(
		vertex_shader,
		GL_VERTEX_SHADER
	);
	shader_sources[1] = shader_source_from_string(
		fragment_shader,
		GL_FRAGMENT_SHADER
	);
	if (!shader_sources[0] ||
	    !shader_sources[1] ||
	    !(shader = shader_new(shader_sources, 2))) {
		errf(ERR_GENERIC, "mesh pipeline shader compile failed");
		return 0;
	} else if (!shader_get_uniforms(shader, uniform_names, uniforms) ||
	           !shader_get_uniform_blocks(shader, uniform_block_names, uniform_blocks)) {
		errf(ERR_GENERIC, "bad mesh pipeline shader");
		return 0;
	}

	// lookup skin transforms array uniform within the uniform block
	const struct ShaderUniform *u = shader_uniform_block_get_uniform(
		&ub_animation,
		"skin_transforms[0]"
	);
	if (!u) {
		return 0;
	}
	u_skin_transforms = *u;

	// allocate an OpenGL buffer for animation uniform block
	glGenBuffers(1, &skin_transforms_buffer);
	if (!skin_transforms_buffer) {
		err(ERR_OPENGL);
		return 0;
	}

	// initialize buffer storage
	glBindBuffer(GL_UNIFORM_BUFFER, skin_transforms_buffer);
	glBufferData(
		GL_UNIFORM_BUFFER,
		ub_animation.size,
		NULL,
		GL_DYNAMIC_DRAW
	);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// check for any OpenGL-related errors
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

static int
configure_texture_mapping(struct MeshProps *props)
{
	int enable_texture_mapping = props->material && props->material->texture;
	int ok = shader_uniform_set(
		&u_enable_texture_mapping,
		1,
		&enable_texture_mapping
	);
	if (enable_texture_mapping) {
		struct Texture *texture = props->material->texture;
		GLint tex_unit = 0;
		ok &= shader_uniform_set(
			&u_texture_map_sampler,
			1,
			&tex_unit
		);
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(texture->type, texture->id);
		if (glGetError() != GL_NO_ERROR) {
			err(ERR_OPENGL);
			return 0;
		}
	}
	return ok;
}

static int
configure_lighting(struct MeshProps *props, struct Light *light, Vec *eye)
{
	int enable_lighting = (
		light &&
		eye &&
		props->material &&
		props->material->receive_light
	);
	int configured = shader_uniform_set(
		&u_enable_lighting,
		1,
		&enable_lighting
	);

	if (enable_lighting) {
		configured &= (
			shader_uniform_set(
				&u_eye,
				1,
				eye
			) &&
			shader_uniform_set(
				&u_material_specular_intensity,
				1,
				&props->material->specular_intensity
			) &&
			shader_uniform_set(
				&u_material_specular_power,
				1,
				&props->material->specular_power
			) &&
			shader_uniform_set(
				&u_light_direction,
				1,
				&light->direction
			) &&
			shader_uniform_set(
				&u_light_color,
				1,
				&light->color
			) &&
			shader_uniform_set(
				&u_light_ambient_intensity,
				1,
				&light->ambient_intensity
			) &&
			shader_uniform_set(
				&u_light_diffuse_intensity,
				1,
				&light->diffuse_intensity
			)
		);
	}

	return configured;
}

static int
configure_shadow_mapping(
	struct MeshProps *props,
	struct Light *light,
	int shadow_map
) {
	int enable_shadow_mapping = (
		props->receive_shadows &&
		light &&
		shadow_map > 0
	);
	int configured = shader_uniform_set(
		&u_enable_shadow_mapping,
		1,
		&enable_shadow_mapping
	);
	if (enable_shadow_mapping) {
		configured &= shader_uniform_set(
			&u_shadow_map_sampler,
			1,
			&shadow_map
		);
		configured &= shader_uniform_set(
			&u_light_space_transform,
			1,
			&light->projection
		);
	}
	return configured;
}

static int
configure_shading(struct MeshProps *props)
{
	Vec color = (
		props->material
		? props->material->color
		: vec(0.7, 0.7, 0.7, 1)
	);
	return shader_uniform_set(
		&u_material_color,
		1,
		&color
	);
}

int
draw_mesh(
	struct Mesh *mesh,
	struct MeshProps *props,
	struct Transform *transform,
	struct Light *light,
	Vec *eye,
	int shadow_map
) {
	assert(mesh != NULL);
	assert(props != NULL);
	assert(transform != NULL);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_model, 1, &transform->model) &&
		shader_uniform_set(&u_view, 1, &transform->view) &&
		shader_uniform_set(&u_projection, 1, &transform->projection) &&
		configure_skinning(
			props->animation,
			shader,
			&u_enable_skinning,
			&u_skin_transforms,
			&ub_animation,
			skin_transforms_buffer
		) &&
		configure_shading(props) &&
		configure_texture_mapping(props) &&
		configure_shadow_mapping(props, light, shadow_map) &&
		configure_lighting(props, light, eye)
	);
	if (!configured) {
		errf(ERR_GENERIC, "failed to configure mesh pipeline");
		return 0;
	}

	glBindVertexArray(mesh->vao);
	glDrawElements(
		GL_TRIANGLES,
		mesh->index_count,
		GL_UNSIGNED_INT,
		(void*)(0)
	);

#ifdef DEBUG
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}
