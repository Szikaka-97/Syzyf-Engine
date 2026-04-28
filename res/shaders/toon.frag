#version 460

in VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
} fs_in;

uniform vec3 lightDir;         
uniform vec4 uColor;            
uniform vec3 camPos;            
uniform float ambientFactor;    
uniform float specularPower;   
uniform float specularIntensity;
uniform float rimThreshold;     
uniform float rimAmount;       
uniform vec3 rimColor;         
uniform float gamma;            

out vec4 fragColor;


const vec3 lightColor = vec3(1.0);
const float lightIntensity = 1.0;
const vec3 specularColor = vec3(0.9);

vec4 toonShadingRim(vec3 n) {
    vec3 dirToEye = normalize(camPos - fs_in.worldPos);
    vec3 halfVec = normalize(dirToEye - lightDir); 
    float df = dot(n, -lightDir); 
    float diffuse = smoothstep(0.0, 0.01, df);
    vec3 light = lightColor * diffuse;

    float sf = dot(n, halfVec);
    sf = pow(sf * diffuse, specularPower * specularPower);
    float sfSmooth = smoothstep(0.005, 0.01, sf);
    vec3 specular = sfSmooth * specularColor * specularIntensity;

    float rimDot = 1.0 - max(dot(dirToEye, n), 0.0);
    float rimIntensity = rimDot * pow(df, rimThreshold);
    rimIntensity = smoothstep(rimAmount - 0.01, rimAmount + 0.01, rimIntensity);
    vec3 rim = rimIntensity * rimColor;

    vec3 ambient = vec3(ambientFactor);

    vec3 color = lightIntensity * uColor.rgb * (light + ambient + rim + specular);
    return vec4(color, uColor.a);
}

vec4 reinhard(vec4 hdr) {
    vec3 ldr = hdr.rgb / (hdr.rgb + 1.0);
    ldr = pow(ldr, vec3(1.0 / gamma));
    return vec4(ldr, hdr.a);
}

void main() {
    vec3 n = normalize(fs_in.normal);
    fragColor = reinhard(toonShadingRim(n));
}