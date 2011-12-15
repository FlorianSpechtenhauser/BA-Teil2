#include "Globals.h"

#include "Scene.h"

#include "Surface.h"
#include "BoundingBox.h"
#include "VolumeRenderer.h"
#include "TextureGrid.h"



Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
	: m_pd3dDevice(pd3dDevice), m_pd3dImmediateContext(pd3dImmediateContext)
{
}

Scene::~Scene()
{
	SAFE_DELETE(m_pBoundingBox);
	SAFE_DELETE(m_pTextureGrid);
	SAFE_DELETE(m_pVolumeRenderer);
}


HRESULT Scene::Initialize(int iTexWidth, int iTexHeight, int iTexDepth)
{
	HRESULT hr;

	// Initialize Shaders
    WCHAR str[MAX_PATH];

	ID3DX11Effect*	pDiffusionEffect;
	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"DiffusionShader11.fx"));
    V_RETURN(CreateEffect(str, &pDiffusionEffect));

	ID3DX11Effect*	pVolumeRenderEffect;
	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"VolumeRenderer.fx"));
	V_RETURN(CreateEffect(str, &pVolumeRenderEffect));

	// Initialize BoundingBox
	m_pBoundingBox = new BoundingBox(m_pd3dDevice, m_pd3dImmediateContext, pDiffusionEffect);
	V_RETURN(m_pBoundingBox->InitSurfaces());
	V_RETURN(m_pBoundingBox->InitBuffers());
	V_RETURN(m_pBoundingBox->InitRasterizerStates());
	V_RETURN(m_pBoundingBox->InitTechniques());
	V_RETURN(m_pBoundingBox->InitRenderTargets(iTexWidth, iTexHeight, iTexDepth));

	// Initialize TextureGrid
	m_pTextureGrid = new TextureGrid(m_pd3dDevice, m_pd3dImmediateContext);
	V_RETURN(m_pTextureGrid->Initialize(iTexWidth, iTexHeight, iTexDepth, pDiffusionEffect->GetTechniqueByName("Grid")));

	// Initialize VolumeRenderer
	m_pVolumeRenderer = new VolumeRenderer(m_pd3dDevice, m_pd3dImmediateContext, pVolumeRenderEffect);
	V_RETURN(m_pVolumeRenderer->Initialize(iTexWidth, iTexHeight, iTexDepth));


	return S_OK;
}

HRESULT Scene::SetScreenSize(int iWidth, int iHeight)
{
    return m_pVolumeRenderer->SetScreenSize(iWidth, iHeight);
}

void Scene::Render(D3DXMATRIX mViewProjection)
{
	m_pBoundingBox->UpdateVertexBuffer();
		
	
	m_pBoundingBox->Render(mViewProjection);
}

void Scene::ChangeControlledSurface()
{
	m_pBoundingBox->ChangeControlledSurface();
}

void Scene::Translate(float fX, float fY, float fZ)
{
	m_pBoundingBox->CSTranslate(fX, fY, fZ);
}

void Scene::RotateX(float fFactor)
{
	m_pBoundingBox->CSRotateX(fFactor);
}

void Scene::RotateY(float fFactor)
{
	m_pBoundingBox->CSRotateY(fFactor);
}

void Scene::Scale(float fFactor)
{
	m_pBoundingBox->CSScale(fFactor);
}


HRESULT Scene::CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect)
{
	HRESULT hr;
	ID3D10Blob *effectBlob = 0, *errorsBlob = 0;
	hr = D3DX11CompileFromFile( name, NULL, NULL, NULL, "fx_5_0", NULL, NULL, NULL, &effectBlob, &errorsBlob, NULL );
	if(FAILED ( hr ))
	{
		std::string errStr((LPCSTR)errorsBlob->GetBufferPointer(), errorsBlob->GetBufferSize());
		WCHAR err[256];
		MultiByteToWideChar(CP_ACP, 0, errStr.c_str(), (int)errStr.size(), err, errStr.size());
		MessageBox( NULL, (LPCWSTR)err, L"Error", MB_OK );
		return hr;
	}
	
	V_RETURN(D3DX11CreateEffectFromMemory(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), 0, m_pd3dDevice, ppEffect));
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT Scene::CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}