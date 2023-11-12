
struct VS_Out
{
    float4 Pos : SV_Position;
    float4 Color : COLOR0;
	float3 Normal : NORMAL;
};

struct PS_Out
{
    float4 Diffuse : SV_Target0;
	float3 Normal : SV_Target1;
	float4 Spec : SV_Target2;
	float4 AO : SV_Target3;
};

PS_Out main(VS_Out IN)
{
	PS_Out output;
	

	output.Diffuse = IN.Color;
	output.Normal = IN.Normal * float3(0.5, 0.5, 0.5) + float3(0.5, 0.5, 0.5);
	output.Spec = float4(6.0 / 255.0, 71.0 / 255.0, 204.0 / 255.0, 1.0);
	output.AO = float4(181.0 / 255.0, 26.0 / 255.0, 0.0, 1.0);
	
    return output;
}
