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

struct VertexShaderOutput
{
	float4 pos					: SV_POSITION;
	float thickness				: THICKNESS;
};

#define DISTRIBUTION_SIZE	1.5
#define NEARPLANE			0.01

[maxvertexcount(4)]
void main(line VertexShaderOutput input[2], inout TriangleStream<GeometryShaderOutput> output)
{
	// it's mostly from here
	// https://atyuwen.github.io/posts/antialiased-line/

	VertexShaderOutput P0 = input[0];
	VertexShaderOutput P1 = input[1];
	
	// swap points if P0 is further away
	if (P0.pos.w > P1.pos.w)
	{
		VertexShaderOutput temp = P0;
		P0 = P1;
		P1 = temp;
	}
	
	// clip lines at nearplane
	if (P0.pos.w < NEARPLANE)
	{
		float ratio = (NEARPLANE - P0.pos.w) / (P1.pos.w - P0.pos.w);
		P0.pos = lerp(P0.pos, P1.pos, ratio);
	}

	// calculate line thickness
	float2 a = P0.pos.xy / P0.pos.w;
	float2 b = P1.pos.xy / P1.pos.w;
	float2 c = P0.thickness * normalize(float2(b.y - a.y, a.x - b.x)) / resolution;
	
	// create quad with 'distance field'
	GeometryShaderOutput g0;
	g0.pos = float4(P0.pos.xy-c*P0.pos.w, P0.pos.zw);
	g0.worldPos = P0.pos;
	g0.tex = float2(-DISTRIBUTION_SIZE, 0.f);
	
	GeometryShaderOutput g1;
	g1.pos = float4(P1.pos.xy-c*P1.pos.w, P1.pos.zw);
	g1.worldPos = P1.pos;
	g1.tex = float2(-DISTRIBUTION_SIZE, 0.f);
	
	GeometryShaderOutput g2;
	g2.pos = float4(P0.pos.xy+c*P0.pos.w, P0.pos.zw);
	g2.worldPos = P0.pos;
	g2.tex = float2(DISTRIBUTION_SIZE, 0.f);

	GeometryShaderOutput g3;
	g3.pos = float4(P1.pos.xy+c*P1.pos.w, P1.pos.zw);
	g3.worldPos = P1.pos;
	g3.tex = float2(DISTRIBUTION_SIZE, 0.f);

	output.Append(g0);
	output.Append(g1);
	output.Append(g2);
	output.Append(g3);
	output.RestartStrip();
}