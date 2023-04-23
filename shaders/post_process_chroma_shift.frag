#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

uniform float u_time;
uniform ivec2 u_resolution;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	vec2 uv = gl_FragCoord.xy / u_resolution.xy;
	float d = length(uv - vec2(0.5,0.5));
	
	// blur amount
	float blur = 0.0;	
	blur = (1.0 + sin(u_time*6.0)) * 0.5;
	blur *= 1.0 + sin(u_time*16.0) * 0.5;
	blur = pow(blur, 3.0);
	blur *= 0.05;
	// reduce blur towards center
	blur *= d;
	
	// final color
    vec3 col;
    col.r = texture( u_texture, vec2(uv.x+blur,uv.y) ).r;
    col.g = texture( u_texture, uv ).g;
    col.b = texture( u_texture, vec2(uv.x-blur,uv.y) ).b;
	
	// scanline
	float scanline = sin(uv.y*800.0)*0.04;
	col -= scanline;
	
	// vignette
	col *= 1.0 - d * 0.5;
	
    color = vec4(col,1.0);
}