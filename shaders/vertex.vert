#version 300 es
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;
layout (location = 3) in vec3 tang;
layout (location = 4) in vec3 bitang;

out vec2 tex_coord;
out vec4 normal;
out vec4 world_pos;

out mat3 tbn;

uniform mat4 model_view_projection;
uniform mat4 model_view;
uniform mat4 model;


void main()
{
    gl_Position = model_view_projection * vec4(pos, 1.0f);
    tex_coord = tex;
    // normal = normalize(model_view * vec4(norm, 1.0f));
    normal = vec4(normalize(norm), 1.0f);
    world_pos = model * vec4(pos, 1.0f);

    // Tangents
    vec3 T = normalize(vec3(model * vec4(tang, 0.0)));
    vec3 N = normalize(vec3(model * vec4(norm, 0.0)));
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);

    tbn = mat3(T, B, N);
}