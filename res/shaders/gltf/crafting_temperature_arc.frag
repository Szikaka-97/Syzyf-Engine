#version 460

#pragma transparent
#pragma no_cull
#pragma no_shadows
#pragma no_depth_prepass

in VS_OUT {
    vec3 localPos;
    vec2 texcoords;
} ps_in;

uniform float uGaugeLocalRadius;
uniform float uInnerRadius01;
uniform float uOuterRadius01;
uniform float uEdgeSoftness01;
uniform float uArcStartDegrees;
uniform float uArcEndDegrees;
uniform float uSafeStartFraction01;
uniform float uSafeEndFraction01;
uniform float uAngleOffsetDegrees;
uniform vec4 uSafeColor;
uniform vec4 uOverheatColor;
uniform vec4 uBaseFillColor;

out vec4 FragColor;

float PositiveAngleDistance(float fromDegrees, float toDegrees){
    return mod(toDegrees - fromDegrees + 360.0, 360.0);
}

void main(){
    float safeRadius = max(uGaugeLocalRadius, 0.0001);
    vec2 centered = ps_in.localPos.xy / safeRadius;

    float radius01 = length(centered);
    float edge = max(uEdgeSoftness01, 0.0001);

    float innerMask = smoothstep(
        uInnerRadius01,
        uInnerRadius01 + edge,
        radius01
    );

    float outerMask = 1.0 - smoothstep(
        uOuterRadius01 - edge,
        uOuterRadius01,
        radius01
    );

    float ringMask = innerMask * outerMask;

    if (ringMask <= 0.001){
        discard;
    }

    // 0 degrees points up, -90 is left, +90 is right.
    float angleDegrees = degrees(atan(centered.x, centered.y)) + uAngleOffsetDegrees;

    float direction = sign(uArcEndDegrees - uArcStartDegrees);
    if (abs(direction) < 0.001){
        direction = 1.0;
    }

    float signedAngle = angleDegrees * direction;
    float signedStart = uArcStartDegrees * direction;
    float signedEnd = uArcEndDegrees * direction;

    float totalLength = PositiveAngleDistance(signedStart, signedEnd);
    float arcPosition = PositiveAngleDistance(signedStart, signedAngle);

    vec4 color = uBaseFillColor;

    if (arcPosition <= totalLength){
        float progress01 = 0.0;
        if (totalLength > 0.0001){
            progress01 = clamp(arcPosition / totalLength, 0.0, 1.0);
        }

        float safeStart01 = clamp(uSafeStartFraction01, 0.0, 1.0);
        float safeEnd01 = clamp(uSafeEndFraction01, safeStart01, 1.0);

        if (progress01 >= safeStart01 && progress01 <= safeEnd01){
            color = uSafeColor;
        }
        else if (progress01 > safeEnd01){
            color = uOverheatColor;
        }
    }

    color.a *= ringMask;

    if (color.a <= 0.001){
        discard;
    }

    FragColor = color;
}
