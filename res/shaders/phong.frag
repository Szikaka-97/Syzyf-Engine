#version 460

// Interfejs zgodny z lit.vert
in VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
} fs_in;

// Uniformy
uniform vec3 lightDir;          // kierunek DO œwiat³a (mo¿e byæ znormalizowany)
uniform vec3 lightColor;        // kolor œwiat³a (domyœlnie bia³y)
uniform vec4 uColor;            // kolor obiektu (ambient + diffuse)
uniform vec3 specularColor;     // kolor odbicia zwierciadlanego (domyœlnie bia³y)
uniform float shininess;        // wyk³adnik po³ysku (np. 32.0)
uniform float ambientStrength;  // natê¿enie œwiat³a otoczenia (0.0 - 1.0)

uniform vec3 camPos;            // pozycja kamery w przestrzeni œwiata

out vec4 fragColor;

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(lightDir);        // kierunek do œwiat³a
    vec3 V = normalize(camPos - fs_in.worldPos); // kierunek do kamery
    vec3 R = reflect(-L, N);             // kierunek odbicia

    // Ambient
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Phong)
    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3 specular = spec * specularColor * lightColor;

    // Kolor koñcowy
    vec3 result = (ambient + diffuse + specular) * uColor.rgb;

    fragColor = vec4(result, uColor.a);
}