cbuffer ConstantBufferData : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 eyePos;
	float2 resolution;
};

struct VertexShaderOutput
{
	float4 pos		: SV_POSITION;
	float thickness : THICKNESS;
};

VertexShaderOutput main(float4 input : POSITION)
{
	VertexShaderOutput output;
	float4 pos = float4(input.xyz, 1.f);

	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	
	output.pos = pos;
	// hack: add the depth bias to the transformed vertex to alleviate depth artifacts
	output.pos.z += -2.f*input.y;
	output.thickness = input.w;
	return output;
}
