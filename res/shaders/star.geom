#version 460

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VARYINGS {
	vec3 normal;
	vec3 worldPos;
} gs_in[];

out VARYINGS {
	vec3 normal;
	vec3 worldPos;
} gs_out;

void main() {
	vec3 aPos = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
	vec3 bPos = gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w;
	vec3 cPos = gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w;

	bool aOutside = aPos.z > 0 && abs(aPos.x) > 1 || abs(aPos.y) > 1;
	bool bOutside = bPos.z > 0 && abs(bPos.x) > 1 || abs(bPos.y) > 1;
	bool cOutside = cPos.z > 0 && abs(cPos.x) > 1 || abs(cPos.y) > 1;

	if (aOutside && bOutside && cOutside) {
		return;
	}

	gl_Position = gl_in[0].gl_Position;
	gs_out.normal = gs_in[0].normal;
	gs_out.worldPos = gs_in[0].worldPos;
    EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	gs_out.normal = gs_in[1].normal;
	gs_out.worldPos = gs_in[1].worldPos;
    EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	gs_out.normal = gs_in[2].normal;
	gs_out.worldPos = gs_in[2].worldPos;
    EmitVertex();

	EndPrimitive();
}