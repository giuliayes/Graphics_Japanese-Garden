#version 410 core

layout (location = 0) in vec3 aOffset;
layout (location = 1) in float aSeed;

uniform mat4 view;
uniform mat4 projection;
uniform float uTime;

uniform vec3 treePosA;
uniform vec3 treePosB;
uniform vec3 treePosC;

out float vAlpha;

float hash(float n)
{
    return fract(sin(n) * 43758.5453);
}

void main()
{
    float seed = aSeed;
    float h = hash(seed);
    vec3 treePos;

    if (h < 0.33)
        treePos = treePosA;
    else if (h < 0.66)
        treePos = treePosB;
    else
        treePos = treePosC;
    float life = fract(uTime * 0.03 + seed);

    float angle = seed * 6.28318;

    float r = sqrt(hash(seed * 7.3));
    float radius = mix(1.5, 4.8, r);  

    vec2 branchOffset;
    branchOffset.x = cos(angle) * radius;
    branchOffset.y = sin(angle) * radius;

    float canopyHeight = hash(seed * 3.1) * 0.6;
    const float CANOPY_BASE = 2.4;

    vec3 pos = treePos + vec3(
        branchOffset.x,
        -CANOPY_BASE + canopyHeight,
        branchOffset.y
    );
    pos.y -= life * 7.5;

    float windStrength = smoothstep(0.0, 1.0, life);

    // Large-scale drifting wind
    float windX = sin(uTime * 0.6 + seed * 4.0) * 4.5;
    float windZ = cos(uTime * 0.5 + seed * 3.5) * 4.0;

    float flutterX = sin(uTime * 7.0 + seed * 30.0) * 0.25;
    float flutterZ = cos(uTime * 6.0 + seed * 28.0) * 0.25;

    pos.x += windX * windStrength + flutterX;
    pos.z += windZ * windStrength + flutterZ;

    gl_Position = projection * view * vec4(pos, 1.0);

    gl_PointSize = mix(18.0, 34.0, hash(seed + 1.3));

    vAlpha =
        smoothstep(0.0, 0.1, life) *
        smoothstep(1.0, 0.7, life);
}