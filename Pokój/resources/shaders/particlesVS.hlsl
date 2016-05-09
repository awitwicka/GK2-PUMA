cbuffer cbView : register(b0) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

struct VSInput
{
	float3 pos : POSITION0;
	float3 pos_prev : POSITION1;
	float age : TEXCOORD0;
	float angle : TEXCOORD1;
	float size : TEXCOORD2;
};

struct GSInput
{
	float4 pos : POSITION0;
	float4 pos_prev : POSITION1;
	float age : TEXCOORD0;
	float angle : TEXCOORD1;
	float size : TEXCOORD2;
};

GSInput main(VSInput i)
{
	GSInput o = (GSInput)0;
	o.pos = float4(i.pos, 1.0f);
	o.pos = mul(viewMatrix, o.pos);
	o.pos_prev = float4(i.pos_prev, 1.0f);
	o.pos_prev = mul(viewMatrix, o.pos_prev);
	o.age = i.age;
	o.angle = i.angle;
	o.size = i.size;
	return o;
}