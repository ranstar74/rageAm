// Adapted from https://www.shadertoy.com/view/Xltfzj

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

float4 Blur(float2 centerCoord, float size, float quality, float directions)
{
    const float pi2 = 6.28;
    
    uint w, h;
    Texture.GetDimensions(w, h);

    float2 radius = size / float2(w, h);
    float4 color = Texture.Sample(Sampler, centerCoord);
    for( float d = 0.0; d < pi2; d += pi2 / directions)
    {
		for (float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality)
        {
            float2 coord = centerCoord + float2(cos(d), sin(d)) * radius * i;
			color += Texture.Sample(Sampler, coord);
        }
    }

    color /= quality * directions - 15.0;
    return color;
}

PS_Out main(VS_Out IN)
{	
    float2 screenCoord = 0.5 * float2(IN.ScreenPosition.x, -IN.ScreenPosition.y) + 0.5;

	// Combine different values to get nice glow effect
    float4 color = float4(0, 0, 0, 0);
    color += Blur(screenCoord, 8, 6, 24) * 1.2;
	color += float4(0.6, 0.6, 0.6, 0);
    color += Blur(screenCoord, 2, 4, 16) * float4(1.2, 1.2, 1.2, 1.2);

	// color = Texture.Sample(Sampler, screenCoord);
	color = lerp(Texture.Sample(Sampler, screenCoord), color, 0.1f);

	PS_Out output;
	output.Diffuse = color;
    return output;
}
