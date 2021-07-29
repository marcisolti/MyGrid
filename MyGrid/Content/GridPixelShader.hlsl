cbuffer ConstantBufferData : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 eyePos;	
	float2 resolution;
};

struct GeometryShaderOutput
{
	float4 pos					: SV_Position;
	float4 worldPos				: WORLDPOS;
	noperspective float2 tex	: TEXCOORDN;
};

//
#define ALPHA	 0.25f
#define FOG_BIAS 0.175f

float4 main(GeometryShaderOutput input) : SV_TARGET
{
	// sample cone filter
	const float aaLine = ALPHA * exp2(-2.7f * input.tex.x * input.tex.x);
	
	// classic exponential fog
	const float depthWithBias = length(input.worldPos.xyz - eyePos.xyz) * FOG_BIAS;
	const float fogValue = exp(-depthWithBias*depthWithBias);
	
	// mix result
	const float foggyLine = lerp(0.f, aaLine, fogValue);

	return float4(0.f, 0.f, 0.f, foggyLine);
} 
