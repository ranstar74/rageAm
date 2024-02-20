
struct VS_In
{
	float3 Pos : POSITION;
};

struct VS_Out
{
	float4 Pos : SV_Position;
	float2 ScreenPosition : TEXCOORD0;
};

VS_Out main(VS_In IN)
{
	VS_Out output;
	output.Pos = float4(IN.Pos, 1.0);
	output.ScreenPosition = float2(IN.Pos.x, IN.Pos.y);
	return output;
}
