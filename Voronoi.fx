//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

matrix    ModelViewProjectionMatrix;
Texture2D flatColorTexture;
Texture2D flatDistTexture;

float4 vBBMin;
float4 vBBMax;

int iSliceIndex;
int iTextureDepth;

//--------------------------------------------------------------------------------------
// Sampler
//--------------------------------------------------------------------------------------

SamplerState linearSamplerBorder
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
};

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------
// RasterizerState 
RasterizerState RS_CullDisabled
{
  MultiSampleEnable = False;
  CullMode = None;
  ScissorEnable = true;
};


// BlendState
BlendState BS_NoBlending
{
  BlendEnable[0] = false;
  RenderTargetWriteMask[0] = 0x0F;
};


// DepthStencilState
DepthStencilState DSS_NonZeroRule
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
    
    //stencil
    StencilEnable = true;
    StencilReadMask = 0x00;
    StencilWriteMask = 0xFF;
    FrontFaceStencilFunc = Always;
    FrontFaceStencilPass = Decr;
    FrontFaceStencilFail = Keep;
    BackFaceStencilFunc = Always;
    BackFaceStencilPass = Incr;
    BackFaceStencilFail = Keep;
    
};

DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	Depthfunc = LESS_EQUAL;
	DepthWriteMask = ALL;
	StencilEnable = FALSE;
};

DepthStencilState DSS_Disabled
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
    
    //stencil
    //StencilEnable = FALSE;
    //StencilReadMask = 0x00;
    //StencilWriteMask = 0x00;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct VS_VORONOI_INPUT
{
	float3 pos		: POSITION;
	float4 color	: COLOR;
};

struct GS_VORONOI_INPUT
{
	float4 pos		: POSITION;
	float4 color	: COLOR;
};

struct GS_VORONOI_OUTPUT
{
	float4 pos		: SV_POSITION; //former vertex positions + vertex positions generated by the geometry shader
	float4 color	: COLOR;
	float4 dist		: TEXTURE0;		// analog zum 2d? (bei VolSurfaces10 Effect_Undistort.fx)
	//float dist		: TEXTURE0;		//analog zum 2d?
};

struct PS_VORONOI_OUTPUT
{
	float4 color : SV_Target0;
	float4 dist	 : SV_Target1;
};

struct VS_RESOLVE_INPUT
{
	float3 pos : POSITION;
	float3 tex : TEXCOORD;
};

struct GS_RESOLVE_INPUT
{
	float4 pos : POSITION;
	float3 tex : TEXCOORD;
};

struct GS_RESOLVE_OUTPUT
{
	float4 pos : SV_Position;
	float3 tex : TEXCOORD;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

struct PS_RESOLVE_OUTPUT
{
	float4 color : SV_Target0;
	float4 dist  : SV_Target1;
};

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------
float4 interpolate(float4 p1, float4 p2, float sliceDepth)
{
	float zDist = p2.z - p1.z;
	float zWeight = (sliceDepth - p1.z)/zDist;
	return lerp(p1, p2, float4(zWeight, zWeight, zWeight, zWeight));
}


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

GS_VORONOI_INPUT VoronoiVS(VS_VORONOI_INPUT input)
{
	GS_VORONOI_INPUT output;
	output.pos = mul(float4(input.pos, 1.0f), ModelViewProjectionMatrix);
	output.color = input.color;
	return output;
}

GS_RESOLVE_INPUT ResolveVS(VS_RESOLVE_INPUT input)
{
	GS_RESOLVE_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------

[maxvertexcount(3)]
void TriangleGS( triangle GS_VORONOI_INPUT input[3], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	GS_VORONOI_OUTPUT output;

	//z-distance of the bounding box
	float zBBDist = vBBMax.z - vBBMin.z;

	//Calculate depth of the current slice
	float sliceDepth = (iSliceIndex/(float)iTextureDepth)*zBBDist+vBBMin.z;
	
	//Calculate triangle normal
	float4 v1 = input[2].pos - input[0].pos;
	float4 v2 = input[1].pos - input[0].pos;
	float3 normal = cross(v1.xyz, v2.xyz);

	//assumed, that all 3 vertices have the same color
	output.color = input[0].color;


	//check if normal is not parallel to the slice
	if(normal.z == 0)
		return;
		
	// check if all points of the triangle have a higher/lower z value as the sliceindex-depth
	if(input[0].pos.z <= sliceDepth && input[1].pos.z <= sliceDepth && input[2].pos.z <= sliceDepth)
	{
		//if normal is pointing away from the slice then invert it
		if(normal.z < 0)
			normal = -normal;
		
		//normalize the normal for the z-value, so we can easily
		//compute the distance-normal vector between the point
		//and the slice
		normal /= normal.z;
		
		for(int v = 0; v < 3; v++)
		{
			//distance of the point to the slice
			float distPosToSliceZ = sliceDepth - input[v].pos.z;
		
			//distance-normal between point of the triangle and slice
			float3 normalToPosAtSlice =	normal*distPosToSliceZ;
			output.pos = float4(input[v].pos.x+normalToPosAtSlice.x, input[v].pos.y+normalToPosAtSlice.y,  length(normalToPosAtSlice), 1.0f);
			output.dist = float4(output.pos.z, 0.0, 0.0, 1.0);
			tStream.Append(output);
		}
		tStream.RestartStrip();
	}
	else if(input[0].pos.z >= sliceDepth && input[1].pos.z >= sliceDepth && input[2].pos.z >= sliceDepth)
	{
		//if normal is pointing away from the slice then invert it
		if(normal.z > 0)
			normal = -normal;
		
		normal /= -normal.z;//normal points in negative z-direction so we have to
							//divide by negative z so we dont change the direction
							//of the normal
			
		for(int v = 0; v < 3; v++)
		{
			//distance of the point to the slice
			float distPosToSliceZ = input[v].pos.z - sliceDepth;

			//distance-normal between point of the triangle and slice
			float3 normalToPosAtSlice =	normal*distPosToSliceZ;
			output.pos = float4(input[v].pos.x+normalToPosAtSlice.x, input[v].pos.y+normalToPosAtSlice.y,  length(normalToPosAtSlice), 1.0f);
			output.dist = float4(0.0, output.pos.z, 0.0, 1.0);
			tStream.Append(output);
		}
		tStream.RestartStrip();
	}
	else
	{
		//divide polygon into 2 polygons, divided by the slice
		//calculate distance function for each polygon

		//case 1: the z-value of one point is the same as the depth of the slice
		//result: 2 triangles
		//NOT SURE IF NEEDED
		for(int v = 0; v < 3; v++)
		{
			if(input[v].pos.z == sliceDepth)
			{
				float4 v0 = input[v].pos;
				v1 = input[(v+1)%3].pos;
				v2 = input[(v+2)%3].pos;
				float3 v3 = v2.xyz - v1.xyz;
				v3 /= abs(v3.z);
				float distToSlice = abs(sliceDepth - v1.z);
				v3 *= distToSlice;//resulting in new vertex

				//create 2 new polygons with
				//v0, v1, v3    v0, v2, v3

				//calculate distance function of both new triangles
			}
		}

		//case 2: no point has a z-value equal to the slice depth
		//result: 3 triangles
		//use approach as in case 1

		//calculate interpolated vectors and create the 3 triangles
		if(input[0].pos.z < sliceDepth)
		{
			//case 2.3
			if(input[1].pos.z < sliceDepth)
			{
				//interpolate between 0 and 2 and between 1 and 2
				//float zDist02 = input[2].pos.z - input[0].pos.z;
				//float zDist12 = input[2].pos.z - input[1].pos.z;
				//float zWeight02 = (sliceDepth - input[0].pos.z)/zDist02;
				//float zWeight12 = (sliceDepth - input[1].pos.z)/zDist12;
				//interVec1 = lerp(input[0].pos, input[2].pos, float4(zWeight02, zWeight02, zWeight02, zWeight02));
				//interVec2 = lerp(input[1].pos, input[2].pos, float4(zWeight12, zWeight12, zWeight12, zWeight12));
				float4 interVec1 = interpolate(input[0].pos, input[2].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0,1,interVec1
				//triangle 2: 1,iV1,iV2
				//triangle 3: 2,iV1,iV2

			}
			//case 2.2
			else if(input[2].pos.z < sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0,2,iV1
				//triangle 2: 2,iV1,iV2
				//triangle 3: 1,iV1,iV2
			}
			//case 1.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[0].pos, input[2].pos, sliceDepth);

				//triangle 1: 1,2,iV2
				//triangle 2: 1,iv1,iv2
				//triangle 3: 0,iv1,iv2
			}
		}
		else
		{
			//case 1.3
			if(input[1].pos.z > sliceDepth)
			{
				//interpolate between 0 and 2 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[2].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0, iv1,iv2
				//triangle 2: 0,1,iv2
				//triangle 3: 2, iv1,iv2
			}
			//case 1.2
			else if(input[2].pos.z > sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0, iv1, iv2
				//triangle 2: 0,2,iv2
				//triangle 3: 1,iv1,iv2
			}
			//case 2.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[0].pos, input[2].pos, sliceDepth);

				//triangle 1: 0,iv1,iv2
				//triangle 2: 1,2,iv1
				//triangle 3: 2,iv1,iv2
			}
		}

		//calculate distance mesh for all 3 triangles
	}
	
}

[maxvertexcount(2)]
void EdgeGS( line GS_VORONOI_INPUT input[2], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	// Slice index as per frame variable

	GS_VORONOI_OUTPUT output;

	for(int v = 0; v < 2; v++)
	{
		output.pos = input[v].pos;
		output.color = input[v].color;
	}

	// check if all points of the edge have a higher/lower z-value as the sliceindex-depth
	// if true
		//calculate/look up distance function
	// else
		//divide edge into 2 edges, divided by the slice
		//calculate distance function for each edge
	tStream.RestartStrip();
}

[maxvertexcount(1)]
void VertexGS( point GS_VORONOI_INPUT input[1], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	// Slice index as per frame variable

	GS_VORONOI_OUTPUT output;
	output.pos = input[0].pos;
	output.color = input[0].color;
	tStream.Append(output);
	// calculate/look up distance function

	tStream.RestartStrip();
}

[maxvertexcount(3)]
void ResolveGS(triangle GS_RESOLVE_INPUT input[3], inout TriangleStream<GS_RESOLVE_OUTPUT> tStream)
{
	GS_RESOLVE_OUTPUT output;
	output.RTIndex = input[0].tex.z;
	for(int v = 0; v < 3; v++)
	{
		output.pos = input[v].pos;
		output.tex = input[v].tex;
		tStream.Append(output);
	}
	tStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

PS_VORONOI_OUTPUT VoronoiPS(GS_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = input.color;
	output.dist = input.dist;
	return output;
}

PS_RESOLVE_OUTPUT ResolvePS(GS_RESOLVE_OUTPUT input)
{
	PS_RESOLVE_OUTPUT output;
	output.color = flatColorTexture.SampleLevel(linearSamplerBorder, input.tex.xy, 0);
	output.dist = flatDistTexture.SampleLevel(linearSamplerBorder, input.tex.xy, 0);
	return output;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 GenerateVoronoiDiagram
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, TriangleGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiPS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
	/*pass P2
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, EdgeGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiPS()));
		//TODO: set states
	}
	pass P3
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, VertexGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiPS()));
		//TODO: set states
	}*/
}

technique10 Flat2DTextureTo3D
{
	pass F2DTTo3D
	{
		SetVertexShader(CompileShader(vs_4_0, ResolveVS()));
		SetGeometryShader(CompileShader(gs_4_0, ResolveGS()));
		SetPixelShader(CompileShader(ps_4_0, ResolvePS()));
		 SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DSS_Disabled, 0 );
	}
}