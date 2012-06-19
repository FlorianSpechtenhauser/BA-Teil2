#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"
#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <FreeImage.h>

Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_vColor = D3DXVECTOR3(0.0, 0.0, 0.0);
	m_vBBMin = D3DXVECTOR3(0.0, 0.0, 0.0);
	m_vBBMax = D3DXVECTOR3(0.0, 0.0, 0.0);

	FreeImage_Initialise();
}


Surface::~Surface()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}

void Surface::Translate(float fX, float fY, float fZ)
{
	m_translation.x += fX;
	m_translation.y += fY;
	m_translation.z += fZ;

	D3DXMATRIX mTrans;
	D3DXMatrixTranslation(&mTrans, fX, fY, fZ);
	D3DXMatrixTranslation(&m_mTrans, m_translation.x, m_translation.y, m_translation.z);
	D3DXMatrixTranslation(&m_mTransInv, -m_translation.x, -m_translation.y, -m_translation.z);

	m_mModel *= mTrans;
}

void Surface::Rotate(D3DXVECTOR3 axis, float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, &axis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

void Surface::RotateX(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationX(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateY(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationY(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateZ(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationZ(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

void Surface::Scale(float fFactor)
{
	D3DXMATRIX mScale;
	D3DXMatrixScaling(&mScale, fFactor, fFactor, fFactor);

	m_mModel *= mScale;
}

void Surface::SetColor(D3DXVECTOR3 vColor)
{
	m_vColor = vColor;
}

void Surface::SetColor(float fR, float fG, float fB)
{
	m_vColor = D3DXVECTOR3(fR, fG, fB);
}

D3DXVECTOR3 Surface::GetColor()
{
	return m_vColor;
}

HRESULT Surface::LoadMesh(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);
	
	//convert LPCWSTR to std::string
	std::string strFileName = ConvertWideCharToChar(lsFileName);
	std::wstring wstrFileName = lsFileName;

	//V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, lsFileName, true));
	D3DXMatrixIdentity(&m_mModel);

	Assimp::Importer Importer;

	const aiScene* pScene = Importer.ReadFile(strFileName.c_str(), aiProcess_Triangulate |
															aiProcess_GenSmoothNormals);

	assert(pScene);

	//std::string errorstring = Importer.GetErrorString();

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	// Get vertex and index count of the whole mesh
	unsigned int mNumVertices = 0;
	unsigned int mNumIndices = 0;
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		mNumVertices += pScene->mMeshes[i]->mNumVertices;
		mNumIndices += pScene->mMeshes[i]->mNumFaces*3;
	}

	// Create vertex and index array
	VERTEX* pVertices = new VERTEX[mNumVertices];
	unsigned int* pIndices = new unsigned int[mNumIndices];
	unsigned int mCurrentVertex = 0;
	unsigned int mCurrentIndex = 0;

	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		for(unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
			const aiVector3D* pTexcoord = &(paiMesh->mTextureCoords[0][j]);

			VERTEX vertex;
			vertex.pos = D3DXVECTOR3(pPos->x, pPos->y, pPos->z);
			vertex.normal = D3DXVECTOR3(pNormal->x, pNormal->y, pNormal->z);
			vertex.texcoord = D3DXVECTOR2(pTexcoord->x, pTexcoord->y);
			pVertices[mCurrentVertex] = vertex;
			mCurrentVertex++;

			if(vertex.pos.x < m_vBBMin.x)
				m_vBBMin.x = vertex.pos.x;
			if(vertex.pos.y < m_vBBMin.y)
				m_vBBMin.y = vertex.pos.y;
			if(vertex.pos.z < m_vBBMin.z)
				m_vBBMin.z = vertex.pos.z;
			if(vertex.pos.x > m_vBBMax.x)
				m_vBBMax.x = vertex.pos.x;
			if(vertex.pos.y > m_vBBMax.y)
				m_vBBMax.y = vertex.pos.y;
			if(vertex.pos.z > m_vBBMax.z)
				m_vBBMax.z = vertex.pos.z;
		}

		for(unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& face = paiMesh->mFaces[j];
			assert(face.mNumIndices == 3);
			pIndices[mCurrentIndex] = face.mIndices[0];
			pIndices[mCurrentIndex+1] = face.mIndices[1];
			pIndices[mCurrentIndex+2] = face.mIndices[2];
			mCurrentIndex += 3;
		}
	}

	m_mNumVertices = mNumVertices;
	m_mNumIndices = mNumIndices;

	//Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = mNumVertices*sizeof(VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vbInitialData;
	vbInitialData.pSysMem = pVertices;
	vbInitialData.SysMemPitch = 0;
	vbInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&vbDesc, &vbInitialData, &m_pVertexBuffer);

	//Create index buffer
	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = mNumIndices*sizeof(unsigned int);
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibInitialData;
	ibInitialData.pSysMem = pIndices;
	ibInitialData.SysMemPitch = 0;
	ibInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibDesc, &ibInitialData, &m_pIndexBuffer);

	strFileName.erase(strFileName.length()-4, 4);
	wstrFileName.erase(wstrFileName.length()-4, 4);

	std::string strTextureName = strFileName.append("_Color.jpg");
	std::wstring wstrTextureName = wstrFileName.append(L"_Color.jpg");

	//Get the image file type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(strTextureName.c_str());

	//load the image file
	FIBITMAP *texture = FreeImage_Load(fif, strTextureName.c_str());
	
	//texture = FreeImage_ConvertTo32Bits(texture);

	if(texture != NULL)
	{
		unsigned char* bits = (unsigned char*)FreeImage_GetBits(texture);

		unsigned int nBPP = FreeImage_GetBPP(texture);
		unsigned int nWidth = FreeImage_GetWidth(texture);
		unsigned int nHeight = FreeImage_GetHeight(texture);
		unsigned int nPitch = FreeImage_GetPitch(texture);

		unsigned char* texturedata = new unsigned char[nWidth*nHeight*4];

		int offset = 0;
		int offset_img = 0;

		for(unsigned int y = 0; y < nHeight; y++)
		{
			for(unsigned int x = 0; x < nWidth; x++)
			{
				texturedata[offset+2] = ((unsigned char*)bits)[offset_img+0];
				texturedata[offset+1] = ((unsigned char*)bits)[offset_img+1];
				texturedata[offset+0] = ((unsigned char*)bits)[offset_img+2];
				texturedata[offset+3] = ((unsigned char*)bits)[offset_img+3];
				offset += 4;
				offset_img += 3;
			}
			offset_img = y * nPitch;
		}


		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.Width = FreeImage_GetWidth(texture);
		texDesc.Height = FreeImage_GetHeight(texture);
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.ArraySize = 1;
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = texturedata;
		texData.SysMemPitch = nWidth*4;
		texData.SysMemSlicePitch = 0;
		V_RETURN(m_pd3dDevice->CreateTexture2D(&texDesc, &texData, &m_pDiffuseTexture));
		DXUT_SetDebugName( m_pDiffuseTexture, strTextureName.c_str());

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDiffuseTexture, &srvDesc, &m_pDiffuseTextureSRV));
	}

	FreeImage_Unload(texture);

	return hr;
}


HRESULT Surface::Initialize(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());

	V_RETURN(LoadMesh(lsFileName));

	return hr;
}

void Surface::Render(D3DXMATRIX mViewProjection)
{
	//Set up global shader variables
    D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

	//Set up vertex and index buffer
	UINT strides = sizeof(VERTEX);
	UINT offsets = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &strides, &offsets );
	m_pd3dImmediateContext->IASetIndexBuffer( m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
    
	//Set input layout
	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//apply pass
		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);
	
		m_pSurfaceTextureVar->SetResource(m_pDiffuseTextureSRV);

		m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);
	}
}

void Surface::RenderVoronoi(ID3DX11EffectTechnique* pVoronoiTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar)
{
	//Set up vertex and index buffer
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = m_pSurfaceMesh.GetVB11( 0, 0 );
    Strides[0] = ( UINT )m_pSurfaceMesh.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    m_pd3dImmediateContext->IASetIndexBuffer( m_pSurfaceMesh.GetIB11( 0 ), m_pSurfaceMesh.GetIBFormat11( 0 ), 0 );


	//Render
    SDKMESH_SUBSET* pSubset = NULL;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	pVoronoiTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pSurfaceTextureVar->SetResource(m_pDiffuseTextureSRV);

	//TRIANGLE
	
	//apply triangle pass
	pVoronoiTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);

	//EDGE

	//apply edge pass
	pVoronoiTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);

	//POINT
	
	//apply point pass
	pVoronoiTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);
}

D3DXVECTOR4 Surface::GetTransformedBBMin()
{
	D3DXVECTOR4 vBBMin;
	D3DXVec3Transform(&vBBMin, &m_vBBMin, &m_mModel);
	return vBBMin;
}

D3DXVECTOR4 Surface::GetTransformedBBMax()
{
	D3DXVECTOR4 vBBMax;
	D3DXVec3Transform(&vBBMax, &m_vBBMax, &m_mModel);
	return vBBMax;
}

HRESULT Surface::InitializeShader()
{
	HRESULT hr(S_OK);

	m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pSurfaceTextureVar = m_pSurfaceEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

   V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

   return hr;
}

