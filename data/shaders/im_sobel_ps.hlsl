// File: im_sobtel_ps.hlsl
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//

// Shader for drawing outlines
// Adapted from https://www.shadertoy.com/view/Mdf3zr
// To be used with im_blit_vs.hlsl

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

float SobelSample(float2 p, float dx, float dy)
{	
	// TODO: Move out to CBuffer
	float d = 0.6f; // Width of the outline
	uint w, h;
	Texture.GetDimensions(w, h);

    float2 uv = p + (float2(dx * d, dy * d) / float2(w, h));
    float4 c = Texture.Sample(Sampler, uv.xy);
	// Any pixel with alpha == 1 is drawn pixel
	// This will produce solid color
	if (c.w == 1.0)
		c = float4(0.5, 0.5, 0.5, 1);
	
    return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
}

PS_Out main(VS_Out IN)
{
	float2 coord = 0.5 * float2(IN.ScreenPosition.x, -IN.ScreenPosition.y) + 0.5;

	float gx = 0.0;
    gx += -1.0 * SobelSample(coord, -1.0, -1.0);
    gx += -2.0 * SobelSample(coord, -1.0,  0.0);
    gx += -1.0 * SobelSample(coord, -1.0,  1.0);
    gx +=  1.0 * SobelSample(coord,  1.0, -1.0);
    gx +=  2.0 * SobelSample(coord,  1.0,  0.0);
    gx +=  1.0 * SobelSample(coord,  1.0,  1.0);
    
    float gy = 0.0;
    gy += -1.0 * SobelSample(coord, -1.0, -1.0);
    gy += -2.0 * SobelSample(coord,  0.0, -1.0);
    gy += -1.0 * SobelSample(coord,  1.0, -1.0);
    gy +=  1.0 * SobelSample(coord, -1.0,  1.0);
    gy +=  2.0 * SobelSample(coord,  0.0,  1.0);
    gy +=  1.0 * SobelSample(coord,  1.0,  1.0);

    float g = gx * gx + gy * gy;
	
	PS_Out output;
	output.Diffuse = float4(1, 1, 0, 1) * g;
    return output;
}
