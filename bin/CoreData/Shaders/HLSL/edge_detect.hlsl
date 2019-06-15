#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

#ifndef D3D11
//D3D9
uniform float cEdgeThreshold;
uniform float4 cEdgeColor;
#else

//D3D11
#ifdef COMPILEPS
cbuffer CustomPS
{
    float cEdgeThreshold;
    float4 cEdgeColor;
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

#ifdef COMPILEPS
float3 normal_from_depth(float depth, float2 texcoords) {
    //screen size comes from depth RT size
    float2 offset1 = float2(0.0, 1.0) * cGBufferInvSize;
    float2 offset2 = float2(1.0, 0.0) * cGBufferInvSize;
    
    float depth1 = Sample2D(EmissiveMap, texcoords + offset1).r;
    float depth2 = Sample2D(EmissiveMap, texcoords + offset2).r;
    
    float3 p1 = float3(offset1, depth1 - depth);
    float3 p2 = float3(offset2, depth2 - depth);
    
    float3 normal = cross(p1, p2);
    normal.z = -normal.z;
    
    return normalize(normal);
}

float4 GetPixelValue(in float2 uv) {
    float depth = Sample2D(EmissiveMap, uv).r;
    return float4(normal_from_depth(depth, uv), depth);
}

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    float4 col = Sample2D(DiffMap, iScreenPos);
    float4 orValue = GetPixelValue(iScreenPos);
    const float2 offsets[8] = {
        float2(-1, -1),
        float2(-1, 0),
        float2(-1, 1),
        float2(0, -1),
        float2(0, 1),
        float2(1, -1),
        float2(1, 0),
        float2(1, 1)
    };
    float4 sampledValue = float4(0,0,0,0);
    for(int j = 0; j < 8; j++) {
        sampledValue += GetPixelValue(iScreenPos + offsets[j] * cGBufferInvSize);
    }
    sampledValue /= 8;
                 
    oColor = lerp(col, cEdgeColor, step(cEdgeThreshold, length(orValue - sampledValue)));
}
#endif
