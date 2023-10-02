
struct VS_Out
{
    float4 Pos : SV_Position;
    float4 Color : COLOR0;
};

float4 main(VS_Out IN) : SV_TARGET
{
    return IN.Color;
}
