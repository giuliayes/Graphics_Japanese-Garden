#version 410 core

in float vAlpha;
out vec4 FragColor;

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float d = length(uv);

    float alpha = smoothstep(1.0, 0.25, d) * vAlpha;

    vec3 color = vec3(1.0, 0.78, 0.88); // sakura pink
    FragColor = vec4(color, alpha);
}
