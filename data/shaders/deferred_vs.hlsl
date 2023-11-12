
cbuffer locals : register(b1)
{
    row_major float4x4	g_ViewProj;
	bool 				g_NoDepth;	// Overlay rendering
	bool				g_Unlit;
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

VS_Out main(VS_In IN)
{
	float4 tPos = mul(float4(IN.Pos, 1), g_ViewProj);
	
	// Set depth near camera near plane
	if (g_NoDepth && (tPos.z / tPos.w) > 0.01)
    {
		tPos /= tPos.w;
		tPos.w = 1.0;
		tPos.z = 0.1;
	}
	
	// Additionally adds slight glowing effect
	float4 col = IN.Color;
	if(g_Unlit)
	{
		const float factor = 2.5f;
		col.x *= factor;
		col.y *= factor;
		col.z *= factor;
	}

    VS_Out output;
    output.Color = col;
	output.Pos = tPos;
	output.Normal = IN.Normal;
	return output;
}
