#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

uniform float u_time;
uniform ivec2 u_resolution;
uniform float u_rate = 1.0f;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

const float range = 0.05;
const float noiseQuality = 250.0;
const float noiseIntensity = 0.0088;
const float offsetIntensity = 0.02;
const float colorOffsetIntensity = 1.3;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float verticalBar(float pos, float uvY, float offset)
{
    float edge0 = (pos - range);
    float edge1 = (pos + range);

    float x = smoothstep(edge0, pos, uvY) * offset;
    x -= smoothstep(pos, edge1, uvY) * offset;
    return x;
}

void main()
{
	vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    for (float i = 0.0; i < 0.71; i += 0.1313)
    {
        float d = mod(u_time * i * u_rate, 1.7);
        float o = sin(1.0 - tan(u_time * 0.24 * i));
    	o *= offsetIntensity;
        uv.x += verticalBar(d, uv.y, o);
    }
    
    float uvY = uv.y;
    uvY *= noiseQuality;
    uvY = float(int(uvY)) * (1.0 / noiseQuality);
    float noise = rand(vec2(u_time * 0.00001, uvY));
    uv.x += noise * noiseIntensity;

    vec2 offsetR = vec2(0.006 * sin(u_time), 0.0) * colorOffsetIntensity;
    vec2 offsetG = vec2(0.0073 * (cos(u_time * 0.97)), 0.0) * colorOffsetIntensity;
    
    float r = texture(u_texture, uv + offsetR).r;
    float g = texture(u_texture, uv + offsetG).g;
    float b = texture(u_texture, uv).b;

    vec4 tex = vec4(r, g, b, 1.0);
    color = tex;
}