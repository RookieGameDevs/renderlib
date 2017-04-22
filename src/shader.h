/******************************************************************************
 *  Shader management and usage classes and APIs.
 *****************************************************************************/
#pragma once

#include <GL/glew.h>
#include <stddef.h>

/**
 * Shader source.
 */
struct ShaderSource {
	GLuint src;
};

struct ShaderSource*
shader_source_from_string(const char *source, GLenum type);

struct ShaderSource*
shader_source_from_file(const char *filename);

void
shader_source_free(struct ShaderSource *ss);

/**
 * Shader uniform.
 */
struct ShaderUniform {
	const char *name;
	GLenum type;
	GLint loc;
	GLuint count;
	GLint offset;
	size_t size;
};

/**
 * Shader uniform value.
 */
struct ShaderUniformValue {
	struct ShaderUniform *uniform;
	size_t count;
	void *data;
};

/**
 * Shader uniform block.
 */
struct ShaderUniformBlock {
	const char *name;
	GLuint index;
	size_t size;
	size_t uniform_count;
	struct ShaderUniform *uniforms;
};

/**
 * Shader vertex attribute.
 */
struct ShaderAttribute {
	const char *name;
	GLuint index;
	GLenum type;
	size_t size;
};

/**
 * Shader program.
 */
struct Shader {
	GLuint prog;
	GLuint uniform_count;
	struct ShaderUniform *uniforms;
	GLuint block_count;
	struct ShaderUniformBlock *blocks;
	GLuint attribute_count;
	struct ShaderAttribute *attributes;
};

struct Shader*
shader_new(struct ShaderSource **sources, unsigned count);

struct Shader*
shader_compile(
	const char *vert_src_filename,
	const char *frag_src_filename,
	const char *uniform_names[],
	struct ShaderUniform *r_uniforms[],
	const char *uniform_block_names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
);

void
shader_free(struct Shader *s);

int
shader_bind(struct Shader *s);

const struct ShaderUniform*
shader_get_uniform(struct Shader *s, const char *name);

int
shader_get_uniforms(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniform *r_uniforms[]
);

const struct ShaderUniformBlock*
shader_get_uniform_block(struct Shader *s, const char *name);

int
shader_get_uniform_blocks(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
);

const struct ShaderUniform*
shader_uniform_block_get_uniform(
	const struct ShaderUniformBlock *block,
	const char *name
);

int
shader_uniform_set(const struct ShaderUniform *uniform, size_t count, ...);
