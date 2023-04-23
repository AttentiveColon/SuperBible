#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

uniform float u_time;
uniform ivec2 u_resolution;
uniform float u_strength;

out vec4 color;

in VS_OUT
{
    vec2 uv;
} fs_in;

void main()
{
	vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    vec4 tex = texture(u_texture, uv);
    
    float strength = u_strength;
    
    float x = (uv.x + 4.0 ) * (uv.y + 4.0 ) * (u_time * 10.0);
	vec4 grain = vec4(mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * strength;
    
	color = tex + grain;
}