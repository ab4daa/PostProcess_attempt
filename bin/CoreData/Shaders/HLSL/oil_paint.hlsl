#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

//http://www.shaderslab.com/demo-63---oil-painting.html

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
    const int _Radius = 2;      //0~10
    float3 mean[4] = {
        float3(0, 0, 0),
        float3(0, 0, 0),
        float3(0, 0, 0),
        float3(0, 0, 0)
    };

    float3 sigma[4] = {
        float3(0, 0, 0),
        float3(0, 0, 0),
        float3(0, 0, 0),
        float3(0, 0, 0)
    };

    float2 start[4] = {
        float2(-_Radius, -_Radius), 
        float2(-_Radius, 0), 
        float2(0, -_Radius), 
        float2(0, 0)
    };
    float2 pos;
    float3 col;
    for (int k = 0; k < 4; k++) {
        for(int i = 0; i <= _Radius; i++) {
            for(int j = 0; j <= _Radius; j++) {
                pos = float2(i, j) + start[k];
                col = Sample2D(DiffMap, iScreenPos + pos * cGBufferInvSize).rgb;
                mean[k] += col;
                sigma[k] += col * col;
            }
        }
    }

    float sigma2;
 
    float n = pow(_Radius + 1, 2);
    float4 color = Sample2D(DiffMap, iScreenPos);
    float min_ = 1;

    for (int l = 0; l < 4; l++) {
        mean[l] /= n;
        sigma[l] = abs(sigma[l] / n - mean[l] * mean[l]);
        sigma2 = sigma[l].r + sigma[l].g + sigma[l].b;

        if (sigma2 < min_) {
            min_ = sigma2;
            color.rgb = mean[l].rgb;
        }
    }
    oColor = color;
}
