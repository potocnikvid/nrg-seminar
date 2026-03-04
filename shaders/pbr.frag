#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;

uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform vec3  camPos;
uniform float exposure;
uniform int   showDiffuse;
uniform int   showSpecular;
uniform int   tonemapMode;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

vec3 tonemap(vec3 c) {
    if (tonemapMode == 1) {
        c = (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);
    } else if (tonemapMode == 2) {
        vec3 x = c;
        c = ((x * (0.15 * x + 0.05) + 0.004) / (x * (0.15 * x + 0.50) + 0.06)) - 0.02 / 0.30;
        vec3 w = vec3(11.2);
        vec3 wd = ((w * (0.15 * w + 0.05) + 0.004) / (w * (0.15 * w + 0.50) + 0.06)) - 0.02 / 0.30;
        c /= wd;
    } else {
        c = c / (c + vec3(1.0));
    }
    return pow(c, vec3(1.0 / 2.2));
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 diffuse = vec3(0.0);
    if (showDiffuse != 0) {
        vec3 irradiance = texture(irradianceMap, N).rgb;
        diffuse = kD * irradiance * albedo;
    }

    vec3 specular = vec3(0.0);
    if (showSpecular != 0) {
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        specular = prefilteredColor * (F * brdf.x + brdf.y);
    }

    vec3 color = diffuse + specular;

    color *= exposure;
    color = tonemap(color);

    FragColor = vec4(color, 1.0);
}
