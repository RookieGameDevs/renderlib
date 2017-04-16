#version 330 core

uniform vec2 size;
uniform mat4 mvp;

out vec2 uv;

// precomputed indexed vertex coordinates
const vec2 positions[4] = vec2[]
(
	vec2(0, 0),  // left-top
	vec2(0, -1), // left-bottom
	vec2(1, 0),  // right-top
	vec2(1, -1)  // right-bottom
);

// precomputed indexed UV coordinates
const vec2 uvs[4] = vec2[]
(
	vec2(0, 1),
	vec2(0, 0),
	vec2(1, 1),
	vec2(1, 0)
);

void
main()
{
	// compute vertex coordinate
	gl_Position = mvp * vec4(positions[gl_VertexID] * size, 0, 1);

	// compute texture coordinate
	uv = uvs[gl_VertexID] * size;
}