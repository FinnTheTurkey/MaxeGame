#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 tex_coord;

layout (shared) uniform Material 
{
    vec3 color;
};

layout (std140) uniform Lights
{
    vec3 light_positions[128];
    vec3 light_directions[128];
    vec3 light_colors[128];
    vec3 light_infos[128];
};

void main()
{
    FragColor = vec4(color.x, color.y, color.z, 1.0f);
    // FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}