cbuffer matrices : register(b1)
{
	row_major float4x4 gWorld;
	row_major float4x4 gWorldView;
	row_major float4x4 gWorldViewProj;
	row_major float4x4 gViewInverse;
}

cbuffer locals : register(b2)
{
	bool gUnlit;
}

struct VS_In
{
    float3 Pos : POSITION;
    float4 Color : COLOR0;
	float3 Normal : NORMAL;
};

struct VS_Out
{
    float4 Pos : SV_Position;
    float4 Color : COLOR0;
	float3 Normal : NORMAL;
};

float DotLigthing(float3 normal, float4 lightPos)
{
	float lighting = dot(normal, lightPos);
	lighting = abs(lighting);
	return lighting;
}

VS_Out main(VS_In IN)
{
	float4 tPos = mul(float4(IN.Pos, 1), gWorldViewProj);
	
	float4 col = IN.Color;
	if(!gUnlit)
	{
		float4 cameraDir = gViewInverse[2];
		float4 light = float4(0, 0, 0, 0);
		light += DotLigthing(IN.Normal, cameraDir);
		light *= 0.97;

		float baseLight = -0.35;
		float4 accumLight = float4(baseLight, baseLight, baseLight, 0);
		accumLight += pow(light, 2) * 0.35;
		accumLight += pow(light, 4) * 0.25;
		accumLight += pow(light, 8) * 0.25;
		col += accumLight;
	}

    VS_Out output;
    output.Color = col;
	output.Pos = tPos;
	output.Normal = IN.Normal;
	return output;
}
