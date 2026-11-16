#version 450

layout(location = 0) in vec3 v_light;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform RedrawMaterialPush
{
    vec4 diffuse;
} material;

void main()
{
    o_color = vec4(v_light, 1.0) * material.diffuse * texture(u_texture, v_uv);
}
