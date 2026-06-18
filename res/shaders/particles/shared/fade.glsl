// Doesn't work well with large quads in the vertex shader
    //  if the performance is bad for smaller particles try moving this there again
    if (proximityFadeMode > 0) {
        float distanceToCamera = length(ps_in.viewPos);
        alpha *= smoothstep(proximityFadeMin, proximityFadeMax, distanceToCamera);
    }

    if (enableDepthFade > 0) {
        vec2 screenSize = vec2(textureSize(depthTex, 0));
        vec2 screenUV = gl_FragCoord.xy / screenSize;
        float rawDepth = texture(depthTex, screenUV).r;
        float sceneDepth = LinearizeDepth(rawDepth);
        float particleDepth = LinearizeDepth(gl_FragCoord.z);
        float distanceToScene = sceneDepth - particleDepth;

        float depthFade = clamp(distanceToScene / depthFadeDistance, 0.0, 1.0);
        alpha *= depthFade;
    }
