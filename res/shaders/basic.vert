layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_UV1) in vec2 vUVCoords;

out vec3 pNormal;
out vec2 pUVCoords;

void main() {
	gl_Position = Object_MVPMatrix * vec4(vPos.xyz, 1.0);
	pNormal = normalize((Object_MVPMatrix * vec4(vNormal.xyz, 0.0)).xyz);
	pUVCoords = vUVCoords;
}