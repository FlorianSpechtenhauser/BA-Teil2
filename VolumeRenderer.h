

class VolumeRenderer
{
public:
	VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);
	~VolumeRenderer();

	HRESULT Initialize(int gridWidth, int gridHeight, int gridDepth, VERTEX vMin, VERTEX vMax);
    HRESULT SetScreenSize(int width, int height);

	void Draw(ID3D11ShaderResourceView * pSourceTexSRV, VERTEX vMin, VERTEX vMax);

	

protected:

	HRESULT InitShaders();
    HRESULT CreateGridBox(VERTEX vMin, VERTEX vMax);
    HRESULT CreateScreenQuad(VERTEX vMin, VERTEX vMax);
	HRESULT CreateRayDataResources(int width, int height);
	void CalculateRenderTextureSize(int screenWidth, int screenHeight);

	void ComputeRayData();
    void ComputeEdgeTexture();
    void DrawBox();
    void DrawScreenQuad();


	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	ID3DX11Effect*			m_pEffect;
	ID3DX11EffectTechnique* m_pTechnique;

	float m_vGridDim[3];
	int m_iMaxDim;

	D3DXMATRIX m_mGridMatrix;

	int m_iRenderTextureWidth;
    int m_iRenderTextureHeight;

    bool m_bUseFP32Blending;

	CHAR sz[100];

	ID3D11InputLayout           *pGridBoxLayout;
    ID3D11Buffer                *pGridBoxVertexBuffer;
    ID3D11Buffer                *pGridBoxIndexBuffer;

	ID3D11InputLayout           *pQuadLayout;
    ID3D11Buffer                *pQuadVertexBuffer;

	ID3D11Texture2D             *pRayDataTex2D[2];
    ID3D11RenderTargetView      *pRayDataRTV[2];
    ID3D11ShaderResourceView    *pRayDataSRV[2];

	ID3D11Texture2D             *pRayDataSmallTex2D;
    ID3D11RenderTargetView      *pRayDataSmallRTV;
    ID3D11ShaderResourceView    *pRayDataSmallSRV;
    ID3D11Texture2D             *pRayCastTex2D;
    ID3D11RenderTargetView      *pRayCastRTV;
    ID3D11ShaderResourceView    *pRayCastSRV;
    ID3D11Texture2D             *pEdgeTex2D;
    ID3D11ShaderResourceView    *pEdgeSRV;
    ID3D11RenderTargetView      *pEdgeRTV;

	// Shader variables
	ID3DX11EffectShaderResourceVariable  *pColorTexVar;
	ID3DX11EffectShaderResourceVariable  *pRayDataVar;
    ID3DX11EffectShaderResourceVariable  *pRayDataSmallVar;
	ID3DX11EffectShaderResourceVariable  *pRayCastVar;
    ID3DX11EffectShaderResourceVariable  *pEdgeVar;

	ID3DX11EffectScalarVariable  *pZNearVar;
    ID3DX11EffectScalarVariable  *pZFarVar;
	ID3DX11EffectScalarVariable  *pGridScaleFactorVar;
	ID3DX11EffectScalarVariable  *pRTWidthVar;
    ID3DX11EffectScalarVariable  *pRTHeightVar;
    
	ID3DX11EffectVectorVariable  *pEyeOnGridVar;

	ID3DX11EffectMatrixVariable  *pWorldViewProjectionVar;
    ID3DX11EffectMatrixVariable  *pInvWorldViewProjectionVar;


};