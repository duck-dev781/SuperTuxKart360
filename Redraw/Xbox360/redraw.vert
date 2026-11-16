#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 v_light;
layout(location = 1) out vec2 v_uv;

layout(push_constant) uniform RedrawVertexPush
{
    mat4 view_projection;
} pc;

void main()
{
    gl_Position = pc.view_projection * vec4(a_position, 1.0);
    float light = max(dot(normalize(a_normal), normalize(vec3(0.35, 0.80, 0.45))), 0.2);
    v_light = vec3(light);
    v_uv = a_uv;
}
