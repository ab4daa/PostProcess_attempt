#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

void VS(float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    const float amount = 0.1;
    const float toRadians = 3.14 / 180;
    float4 color = Sample2D(DiffMap, iScreenPos);

    float randomIntensity =
        frac( 10000 * sin(
                ( iScreenPos.x / cGBufferInvSize.x
                + iScreenPos.y / cGBufferInvSize.y
                * cElapsedTimePS) * toRadians)
        );

    randomIntensity *= amount;
    color.rgb += randomIntensity;
    oColor = color;
}
