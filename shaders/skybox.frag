#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float exposure;
uniform int   tonemapMode;

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

void main() {
    vec3 envColor = texture(environmentMap, localPos).rgb;
    envColor *= exposure;
    envColor = tonemap(envColor);
    FragColor = vec4(envColor, 1.0);
}
