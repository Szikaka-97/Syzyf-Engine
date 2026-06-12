    mat.metallic = arm.b * metallicFactor;
	mat.roughness = arm.g * roughnessFactor;

  // Ambient Occlusion
  float ao = 1.0f;
  vec2 screenUV = gl_FragCoord.xy / Global_Resolution.xy; 
  float ssao = texture(Builtin_AOMap, screenUV).r;

  if (useOcclusion) {
    ao = arm.r;
  }
  ao = ao * ssao;

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, mat.albedo, mat.metallic);

	fragColor = vec4(
		shade(
			mat,
			ps_in.worldPos,
			N,
			vec3(0, 0, 0)
		),
		alpha
	);

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, mat.roughness);

	vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - mat.metallic;

	vec3 irradiance = texture(Builtin_EnvIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * mat.albedo;

	const float MAX_REFLECTION_LOD = 7.0;
    vec3 prefilteredColor = textureLod(Builtin_EnvPrefilterMap, R, mat.roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(Builtin_BRDFConvolutionMap, vec2(max(dot(N, V), 0.0), mat.roughness)).rg;
	
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
	vec3 ambient = (kD * diffuse + specular) * ao + mat.albedo * (Light_AmbientLight.xyz * Light_AmbientLight.w);
	vec3 emissive = texture(emissiveMap, texCoords).xyz * emissiveFactor * emissiveStrength;

  fragColor.xyz += ambient + emissive;
