
cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 g_ViewProj;
}

struct VS_In
{
    float3 Pos : POSITION;
    float4 Color : COLOR0;
};

struct VS_Out
{
    float4 Pos : SV_Position;
    float4 Color : COLOR0;
};

VS_Out main(VS_In IN)
{
    VS_Out output;
    output.Pos = mul(float4(IN.Pos, 1), g_ViewProj);
    output.Color = IN.Color;
    return output;
}
