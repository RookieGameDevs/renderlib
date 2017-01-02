#version 330 core

uniform sampler2DRect atlas_map_sampler;
uniform uint atlas_offset;
uniform vec4 color = vec4(1.0);
uniform float opacity = 1.0;

flat in uint char;
in vec2 uv;
out vec4 out_color;

void main()
{
	float s = uv.s + char * atlas_offset;
	float t = uv.t;
	out_color = color * vec4(texture(atlas_map_sampler, vec2(s, t)).r) * opacity;
}
