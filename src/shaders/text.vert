#version 330 core

layout(location=0) in vec2 in_coord;
layout(location=1) in uint in_char;

uniform mat4 mvp;
uniform usampler1D glyph_map_sampler;

out vec2 uv;
flat out uint char;

void main()
{
	char = in_char;

	vec4 glyph = vec4(texelFetch(glyph_map_sampler, int(in_char), 0));

	// compute vertex coordinate based on vertex ID
	float x = (gl_VertexID % 2) * glyph.r;
	float y = (gl_VertexID < 2 ? 0 : 1) * glyph.g;

	// compute UV
	uv.s = x;
	uv.t = y;

	// compute position
	gl_Position = mvp * vec4(in_coord.x + x, in_coord.y + y, 0, 1);
}
