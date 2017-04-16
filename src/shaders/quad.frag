#version 330 core

in vec2 uv;
out vec4 out_color;

uniform vec4 color = vec4(1);
uniform vec2 size;
uniform uvec4 border;
uniform sampler2DRect texture_sampler;
uniform float opacity = 1.0;
uniform bool enable_texture_mapping = false;

float
clamp_coord(float v, float size, float texture_size, uint lo, uint hi)
{
	if (v > lo) {
		if (v >= size - hi) {
			v -= size - hi;
			v += texture_size - hi;
		} else {
			v -= lo;
			v = lo + uint(v) % uint(texture_size - lo - hi);
		}
	}
	return v;
}

void
main()
{
	out_color = color;
	if (enable_texture_mapping) {
		ivec2 texture_size = textureSize(texture_sampler);
		vec2 norm_uv = vec2(
			clamp_coord(uv.x, size.x, texture_size.x, border.r, border.g),
			clamp_coord(size.y - uv.y, size.y, texture_size.y, border.b, border.a)
		);
		out_color *= texture(texture_sampler, norm_uv);
	}
	out_color *= opacity;
}