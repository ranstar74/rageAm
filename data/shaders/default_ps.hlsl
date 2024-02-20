
struct VS_Out
{
    float4 Pos : SV_Position;
    float4 Color : COLOR0;
	float3 Normal : NORMAL;
};

struct PS_Out
{
    float4 Diffuse : SV_Target0;
};

PS_Out main(VS_Out IN)
{
	PS_Out output;
	output.Diffuse = IN.Color;
    return output;
}
