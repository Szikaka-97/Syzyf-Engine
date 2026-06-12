vec3 getNormalFromMap() {
	vec3 tangentNormal = texture(normalMap, ps_in.texcoords).xyz * 2.0 - 1.0;
	vec3 N   = normalize(ps_in.normal);
	vec3 T  = normalize(ps_in.tangent.xyz);
	vec3 B  = normalize(cross(N, T)) * ps_in.tangent.w;
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}
