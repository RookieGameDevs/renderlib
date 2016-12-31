#include "error.h"
#include "file_utils.h"
#include "shader.h"
#include "string_utils.h"
#include <assert.h>
#include <matlib.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static size_t
compute_uniform_size(struct ShaderUniform *uniform)
{
	switch (uniform->type) {
	case GL_INT:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_2D_RECT:
	case GL_SAMPLER_1D:
	case GL_INT_SAMPLER_1D:
	case GL_UNSIGNED_INT_SAMPLER_1D:
		return uniform->count * sizeof(GLint);
	case GL_BOOL:
		return uniform->count * sizeof(GLboolean);
	case GL_UNSIGNED_INT:
		return uniform->count * sizeof(GLuint);
	case GL_FLOAT:
		return uniform->count * sizeof(GLfloat);
	case GL_FLOAT_MAT4:
		return uniform->count * sizeof(GLfloat) * 16;
	case GL_FLOAT_VEC4:
		return uniform->count * sizeof(GLfloat) * 4;
	case GL_FLOAT_VEC3:
		return uniform->count * sizeof(GLfloat) * 3;
	case GL_FLOAT_VEC2:
		return uniform->count * sizeof(GLfloat) * 2;
	default:
		err(ERR_SHADER_UNKNOWN_UNIFORM_TYPE);
	}
	return 0;  // unknown uniform type
}

struct ShaderSource*
shader_source_from_string(const char *source, GLenum type)
{
	assert(source != NULL);
	assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);

	// alloc ShaderSource struct
	struct ShaderSource *ss = malloc(sizeof(struct ShaderSource));
	if (!ss) {
		err(ERR_NO_MEM);
		return NULL;
	}

	// create the shader
	if (!(ss->src = glCreateShader(type))) {
		err(ERR_OPENGL);
		goto error;
	}

	// set shader source and compile it
	glShaderSource(ss->src, 1, (const char**)&source, NULL);
	glCompileShader(ss->src);
	GLint status;
	glGetShaderiv(ss->src, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		// fetch compile log
		int log_len;
		glGetShaderiv(ss->src, GL_INFO_LOG_LENGTH, &log_len);
		char log[log_len];
		glGetShaderInfoLog(ss->src, log_len, NULL, log);
		errf(ERR_SHADER_COMPILE, "%s", log);
		goto error;
	}

	return ss;

error:
	shader_source_free(ss);
	return NULL;
}

struct ShaderSource*
shader_source_from_file(const char *filename)
{
	assert(filename != NULL);

	struct ShaderSource *ss = NULL;
	char *source = NULL;

	// determine shader type from filename extension
	GLenum type = 0;
	const char *ext = strrchr(filename, '.');
	if (strncmp(ext, ".vert", 4) == 0) {
		type = GL_VERTEX_SHADER;
	} else if (strncmp(ext, ".frag", 4) == 0) {
		type = GL_FRAGMENT_SHADER;
	}

	// attempt to read the file and compile its content as shader
	if (!type ||
	    !file_read(filename, &source) ||
	    !(ss = shader_source_from_string(source, type))) {
		errf(ERR_INVALID_SHADER, "%s", filename);
		goto error;
	}

cleanup:
	free(source);

	return ss;

error:
	shader_source_free(ss);
	ss = NULL;
	goto cleanup;
}

void
shader_source_free(struct ShaderSource *ss)
{
	if (ss) {
		glDeleteShader(ss->src);
		free(ss);
	}
}

static int
query_uniform_block(
	struct Shader *shader,
	GLuint index,
	size_t max_name_len
) {
	struct ShaderUniformBlock *block = &shader->blocks[index];

	// query uniform block name
	char name[max_name_len];
	glGetActiveUniformBlockName(
		shader->prog,
		index,
		max_name_len,
		NULL,
		name
	);
	if (!(block->name = string_copy(name))) {
		err(ERR_NO_MEM);
		return 0;
	}

	// query block index
	block->index = glGetUniformBlockIndex(shader->prog, name);
	if (block->index == GL_INVALID_INDEX) {
		errf(ERR_SHADER_NO_UNIFORM_BLOCK, "%s", name);
		return 0;
	}

	// query block size
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_DATA_SIZE,
		(GLint*)&block->size
	);

	// query the number of active uniforms within the block and allocate
	// space for their indices and query them
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
		(GLint*)&block->uniform_count
	);
	GLuint uniform_indices[block->uniform_count];
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
		(GLint*)uniform_indices
	);

	// allocate block uniforms array
	size_t uniforms_size = sizeof(struct ShaderUniform) * block->uniform_count;
	if (!(block->uniforms = malloc(uniforms_size))) {
		err(ERR_NO_MEM);
		return 0;
	}
	memset(block->uniforms, 0, uniforms_size);

	// query uniform sizes
	GLint uniform_sizes[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_SIZE,
		uniform_sizes
	);

	// query uniform types
	GLint uniform_types[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_TYPE,
		uniform_types
	);

	// query uniform offsets within the block
	GLint uniform_offsets[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_OFFSET,
		uniform_offsets
	);

	// query uniform name lengths
	GLint uniform_name_lengths[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_NAME_LENGTH,
		uniform_name_lengths
	);

	// query the information about each block uniform
	for (size_t i = 0; i < block->uniform_count; i++) {
		struct ShaderUniform *uniform = &block->uniforms[i];
		uniform->count = uniform_sizes[i];
		uniform->type = uniform_types[i];
		uniform->offset = uniform_offsets[i];
		uniform->loc = -1;
		if (!(uniform->size = compute_uniform_size(uniform))) {
			return 0;
		}

		// retrieve uniform name
		if (!(uniform->name = malloc(uniform_name_lengths[i]))) {
			err(ERR_NO_MEM);
			return 0;
		}
		glGetActiveUniformName(
			shader->prog,
			uniform_indices[i],
			uniform_name_lengths[i],
			NULL,
			(GLchar*)uniform->name
		);
	}

	return 1;
}

static int
init_shader_uniform_blocks(struct Shader *s)
{
	// query the number of uniform blocks
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORM_BLOCKS, (GLint*)&s->block_count);
	if (s->block_count == 0) {
		// it's ok to have no uniform blocks
		return 1;
	}

	// allocate uniform blocks array
	size_t blocks_size = sizeof(struct ShaderUniformBlock) * s->block_count;
	s->blocks = malloc(blocks_size);
	if (!s->blocks) {
		err(ERR_NO_MEM);
		return 0;
	}
	memset(s->blocks, 0, blocks_size);

	// query maximum block name length
	size_t max_name_len = 0;
	glGetProgramiv(
		s->prog,
		GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH,
		(GLint*)&max_name_len
	);

	// populate blocks array
	for (size_t i = 0; i < s->block_count; i++) {
		if (!query_uniform_block(s, i, max_name_len)) {
			return 0;
		}
	}
	return 1;
}

static int
init_shader_uniforms(struct Shader *s)
{
	// query the number of uniforms in shader proram
	GLuint count = 0;
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORMS, (GLint*)&count);
	if (count == 0) {
		s->uniform_count = 0;
		s->uniforms = NULL;
		return 1;
	}

	// prepare name string buffer
	GLsizei name_len;
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, (GLint*)&name_len);
	GLchar name[name_len];

	// query information about each uniform
	size_t uniform_sizes[count];
	GLenum uniform_types[count];
	GLint uniform_locations[count];
	char* uniform_names[count];
	size_t actual_count = 0;
	for (size_t i = 0; i < count; i++) {
		// query the size, type and name information
		glGetActiveUniform(
			s->prog,
			i,
			name_len,
			NULL,
			(GLint*)&uniform_sizes[actual_count],
			&uniform_types[actual_count],
			(GLchar*)name
		);

		// only uniforms which have a location different from -1 are to
		// be stored, others belong to uniform blocks
		GLint loc = glGetUniformLocation(s->prog, (GLchar*)name);
		if (loc != -1) {
			uniform_names[actual_count] = string_copy(name);
			uniform_locations[actual_count] = loc;
			actual_count++;
		}
	}

	// fill uniforms array
	if (actual_count > 0) {
		size_t uniforms_size = sizeof(struct ShaderUniform) * actual_count;
		s->uniforms = malloc(uniforms_size);
		if (!s->uniforms) {
			err(ERR_NO_MEM);
			return 0;
		}
		memset(s->uniforms, 0, uniforms_size);
		s->uniform_count = actual_count;
		for (size_t i = 0; i < actual_count; i++) {
			struct ShaderUniform *uniform = &s->uniforms[i];
			uniform->name = uniform_names[i];
			uniform->loc = uniform_locations[i];
			uniform->type = uniform_types[i];
			uniform->count = uniform_sizes[i];
			uniform->offset = -1;
			if (!(uniform->size = compute_uniform_size(uniform))) {
				return 0;
			}
		}
	}

	return 1;
}

struct Shader*
shader_new(struct ShaderSource **sources, unsigned count)
{
	assert(sources != NULL);
	assert(count > 0);

	GLuint prog = 0;
	struct Shader *shader = NULL;

	// create shader program
	prog = glCreateProgram();
	if (!prog) {
		err(ERR_OPENGL);
		goto error;
	}

	// attach shaders and link the program
	for (unsigned i = 0; i < count; i++) {
		assert(sources[i]->src != 0);
		glAttachShader(prog, sources[i]->src);
	}
	glLinkProgram(prog);

	// retrieve link status
	int status = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		// retrieve link log
		int log_len;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
		char log[log_len];
		glGetProgramInfoLog(prog, log_len, NULL, log);
		errf(ERR_SHADER_LINK, "%s", log);
		goto error;
	}

	// alloc Shader struct
	shader = malloc(sizeof(struct Shader));
	if (!shader) {
		err(ERR_NO_MEM);
		goto error;
	}
	memset(shader, 0, sizeof(struct Shader));
	shader->prog = prog;

	// initialize shader uniforms and uniform blocks tables
	if (!init_shader_uniform_blocks(shader) ||
	    !init_shader_uniforms(shader)) {
		goto error;
	}

	return shader;

error:
	shader_free(shader);
	return NULL;
}

struct Shader*
shader_compile(
	const char *vert_src_filename,
	const char *frag_src_filename,
	const char *uniform_names[],
	struct ShaderUniform *r_uniforms[],
	const char *uniform_block_names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
) {
	assert(vert_src_filename != NULL);
	assert(frag_src_filename != NULL);
	assert(uniform_names ? r_uniforms != NULL : 1);
	assert(uniform_block_names ? r_uniform_blocks != NULL : 1);

	// compile sources and link them into a shader program
	struct Shader *shader = NULL;
	struct ShaderSource *sources[2] = { NULL, NULL };
	sources[0] = shader_source_from_file(vert_src_filename);
	sources[1] = shader_source_from_file(frag_src_filename);
	if (!sources[0] || !sources[1] || !(shader = shader_new(sources, 2))) {
		goto error;
	}

	int ok = 1;

	// lookup uniforms
	if (uniform_names) {
		ok &= shader_get_uniforms(shader, uniform_names, r_uniforms);
	}

	// lookup uniform blocks
	if (uniform_block_names) {
		ok &= shader_get_uniform_blocks(
			shader,
			uniform_block_names,
			r_uniform_blocks
		);
	}

	if (!ok) {
		goto error;
	}

	// TODO: return the compiled shader sources

	return shader;

error:
	shader_source_free(sources[0]);
	shader_source_free(sources[1]);
	shader_free(shader);
	return NULL;
}

void
shader_free(struct Shader *s)
{
	if (s) {
		for (size_t i = 0; i < s->uniform_count; i++) {
			free((char*)s->uniforms[i].name);
		}
		free(s->uniforms);

		for (size_t i = 0; i < s->block_count; i++) {
			struct ShaderUniformBlock *block = &s->blocks[i];
			for (size_t j = 0; j < block->uniform_count; j++) {
				free((char*)block->uniforms[j].name);
			}
			free(block->uniforms);
		}
		free(s->blocks);
		free(s);
	}
}

int
shader_bind(struct Shader *s)
{
	assert(s != NULL);

	glUseProgram(s->prog);

#ifdef DEBUG
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif

	return 1;
}

const struct ShaderUniform*
shader_get_uniform(struct Shader *s, const char *name)
{
	assert(s != NULL);
	assert(name != NULL);

	for (GLuint i = 0; i < s->uniform_count; i++) {
		struct ShaderUniform *uniform = s->uniforms + i;
		if (strcmp(uniform->name, name) == 0) {
			return uniform;
		}
	}
	errf(ERR_SHADER_NO_UNIFORM, "%s", name);
	return NULL;
}

int
shader_get_uniforms(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniform *r_uniforms[]
) {
	for (size_t i = 0; names[i] != NULL; i++) {
		const struct ShaderUniform *u = shader_get_uniform(
			shader,
			names[i]
		);
		if (!u) {
			return 0;
		}
		*r_uniforms[i] = *u;
	}
	return 1;
}

const struct ShaderUniformBlock*
shader_get_uniform_block(struct Shader *s, const char *name)
{
	assert(s != NULL);
	assert(name != NULL);

	for (GLuint i = 0; i < s->block_count; i++) {
		struct ShaderUniformBlock *block = &s->blocks[i];
		if (strcmp(block->name, name) == 0) {
			return block;
		}
	}
	errf(ERR_SHADER_NO_UNIFORM_BLOCK, "%s", name);
	return NULL;
}

int
shader_get_uniform_blocks(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
) {
	for (size_t i = 0; names[i] != NULL; i++) {
		const struct ShaderUniformBlock *ub = shader_get_uniform_block(
			shader,
			names[i]
		);
		if (!ub) {
			return 0;
		}
		*r_uniform_blocks[i] = *ub;
	}
	return 1;
}

const struct ShaderUniform*
shader_uniform_block_get_uniform(const struct ShaderUniformBlock *block, const char *name)
{
	assert(block != NULL);
	assert(name != NULL);

	for (size_t i = 0; i < block->uniform_count; i++) {
		if (strcmp(block->uniforms[i].name, name) == 0) {
			return &block->uniforms[i];
		}
	}
	errf(ERR_SHADER_NO_UNIFORM, "%s", name);
	return NULL;
}

int
shader_uniform_set(const struct ShaderUniform *uniform, size_t count, ...)
{
	assert(uniform->loc != -1);
	assert(uniform->count <= count);

	va_list ap;
	va_start(ap, count);

	switch (uniform->type) {
	case GL_INT:
	case GL_BOOL:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_2D_RECT:
	case GL_SAMPLER_1D:
	case GL_INT_SAMPLER_1D:
	case GL_UNSIGNED_INT_SAMPLER_1D:
		glUniform1iv(uniform->loc, count, va_arg(ap, GLint*));
		break;

	case GL_UNSIGNED_INT:
		glUniform1uiv(uniform->loc, count, va_arg(ap, GLuint*));
		break;

	case GL_FLOAT:
		glUniform1fv(uniform->loc, count, va_arg(ap, GLfloat*));
		break;

	case GL_FLOAT_MAT4:
		glUniformMatrix4fv(uniform->loc, count, GL_TRUE, va_arg(ap, GLfloat*));
		break;

	case GL_FLOAT_VEC4:
		glUniform4fv(uniform->loc, count, va_arg(ap, GLfloat*));
		break;

	case GL_FLOAT_VEC3:
		{
			GLfloat data[count][3];
			Vec *v = va_arg(ap, Vec*);
			for (size_t i = 0; i < count; i++) {
				memcpy(data[i], v + i, sizeof(GLfloat) * 3);
			}
			glUniform3fv(uniform->loc, count, (GLfloat*)data);
		}
		break;
	case GL_FLOAT_VEC2:
		{
			GLfloat data[count][2];
			Vec *v = va_arg(ap, Vec*);
			for (size_t i = 0; i < count; i++) {
				memcpy(data[i], v + i, sizeof(GLfloat) * 2);
			}
			glUniform2fv(uniform->loc, count, (GLfloat*)data);
		}
		break;
	}
	va_end(ap);

#ifdef DEBUG
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}
