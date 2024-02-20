
struct VS_Out
{
	float4 Pos : SV_Position;
	float2 ScreenPosition : TEXCOORD0;
};

struct PS_Out
{
    float4 Diffuse : SV_Target0;
};

Texture2D Texture : register(t0);
sampler Sampler : register(s0);

PS_Out main(VS_Out IN)
{
	PS_Out output;
	float2 screenCoord = 0.5 * float2(IN.ScreenPosition.x, -IN.ScreenPosition.y) + 0.5;
	output.Diffuse = Texture.Sample(Sampler, screenCoord);
    return output;
}
