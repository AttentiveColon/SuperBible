#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	color = texture(u_texture, fs_in.uv);
	color = color.bgra;
}
