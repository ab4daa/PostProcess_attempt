#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

#ifndef D3D11
//D3D9
uniform float cPosterizationLevel;

#else

//D3D11
#ifdef COMPILEPS
cbuffer CustomPS
{
    float cPosterizationLevel;
}
#endif
#endif

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
    float4 texColor = Sample2D(DiffMap, iScreenPos);
    float avg = (texColor.r + texColor.g + texColor.b) / 3.0;
    float3 grey  = float3(avg, avg, avg);
    float3 grey1 = grey;

    grey = floor(grey * cPosterizationLevel) / cPosterizationLevel;

    texColor.rgb += (grey - grey1);

    oColor = texColor;
}