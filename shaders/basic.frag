#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

// matrices
uniform mat4 view;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform bool useDirectionalLight;

uniform vec3 lightPos2;
uniform vec3 lightColor2;

// material
uniform sampler2D diffuseTexture;
uniform vec3 materialDiffuse;
uniform int hasDiffuseTexture;

// fog
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec2 fogCenterXZ;
uniform float fogInnerRadius;
uniform float fogOuterRadius;

// shadows
uniform sampler2D shadowMap;
uniform bool useShadows;

// lighting params
float ambientStrength = 0.10;
float specularStrength = 0.50;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.0025;
    return (currentDepth - bias > closestDepth) ? 0.75 : 0.0;
}

void main()
{
    vec3 normal = normalize(normalMatrix * fNormal);
    vec3 viewPos = vec3(view * vec4(fPosition, 1.0));
    vec3 viewDir = normalize(-viewPos);

    vec3 baseColor = (hasDiffuseTexture == 1)
        ? texture(diffuseTexture, fTexCoords).rgb
        : materialDiffuse;

    vec3 sunLight = vec3(0.0);

    if (useDirectionalLight)
    {
        vec3 lightDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));

        vec3 ambient = ambientStrength * lightColor;

        float diff = max(dot(normal, lightDirEye), 0.0);
        vec3 diffuse = diff * lightColor;

        vec3 reflectDir = reflect(-lightDirEye, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = specularStrength * spec * lightColor;

        float shadow = 0.0;
        if (useShadows)
            shadow = ShadowCalculation(fragPosLightSpace);

        sunLight = ambient + (1.0 - shadow) * (diffuse + specular);
    }

    vec3 lampLight = vec3(0.0);
    if (length(lightColor2) > 0.001)
    {
        vec3 lp = vec3(view * vec4(lightPos2, 1.0));
        vec3 ldir = normalize(lp - viewPos);
        float d = length(lp - viewPos);

        float atten = 1.0 / (1.0 + 0.15 * d + 0.03 * d * d);
        float pdiff = max(dot(normal, ldir), 0.0);

        lampLight = pdiff * lightColor2 * atten;
    }

    vec3 litColor = (sunLight + lampLight) * baseColor;

    float distXZ = distance(fPosition.xz, fogCenterXZ);
    float radialFog = smoothstep(fogInnerRadius, fogOuterRadius, distXZ);

    float depth = length(viewPos);
    float depthFog = 1.0 - exp(-depth * fogDensity);

    float fogFactor = clamp(radialFog * depthFog, 0.0, 1.0);

    vec3 finalColor = mix(litColor, fogColor, fogFactor);
    fColor = vec4(finalColor, 1.0);
}
