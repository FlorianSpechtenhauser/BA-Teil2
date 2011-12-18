#include "Globals.h"
#include "Scene.h"
#include "resource.h"
#include <string>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera          g_Camera;               // A model viewing camera
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D   
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Scene
Scene*						g_pScene;

D3DXVECTOR3                 g_Eye = D3DXVECTOR3( 0.0f, 0.0f, -20.0f );
D3DXVECTOR3                 g_At = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3                 g_Up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

// Global variables
bool						g_useFire = false;;
int							g_Width = 800;
int							g_Height = 600;
float						g_zNear = 0.05f;
float						g_zFar = 1000.0f;
bool						g_renderGlow = false;
float						g_glowContribution = 0.81f;
float						g_finalIntensityScale = 28.0f; 
float						g_finalAlphaScale = 0.95f;
float						g_smokeColorMultiplier = 2.0f;   
float						g_smokeAlphaMultiplier = 0.05f; 
int							g_RednessFactor = 5; 
float						g_xyVelocityScale = 4.8f;
float						g_zVelocityScale = 4.0f;
D3DXMATRIX					g_View;
D3DXMATRIX					g_Projection;
float						g_Fovy = D3DX_PI * 0.25f;

ID3D11Texture2D*            g_pSceneDepthTex2D      = NULL;
ID3D11Texture2D*            g_pSceneDepthTex2DNonMS = NULL;
ID3D11ShaderResourceView*   g_pSceneDepthSRV        = NULL;
ID3D11RenderTargetView*     g_pSceneDepthRTV        = NULL;

D3DXMATRIX                  g_gridWorld;

// Control parameters
int							g_mouseX = 0;
int							g_mouseY = 0;
int							g_mouseSpeed = 8;
bool						g_bRotatesWithMouse = true;
bool						g_bCameraActive = false;

float						g_fElapsedTime = 0;

int							g_iTextureWidth = 200;
int							g_iTextureHeight = 200;
int							g_iTextureDepth = 200;
bool						g_bBlockMouseDragging = false;

// Texthelper
CDXUTTextHelper*            g_pTxtHelper = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN		1
#define IDC_TOGGLEREF				2
#define IDC_CHANGEDEVICE			3

#define IDC_CHANGE_CONTROL			4
#define IDC_ROTATE_MOVE_CAMERA		5
#define IDC_ROTATE					6
#define IDC_MOVE					7
#define IDC_CAMERA					8
#define IDC_CHANGE_TEXTRES			9
#define IDC_TEXTRES_WIDTH_STATIC	10
#define IDC_TEXTRES_WIDTH_SLIDER	11
#define IDC_TEXTRES_HEIGHT_STATIC	12
#define IDC_TEXTRES_HEIGHT_SLIDER	13
#define IDC_TEXTRES_DEPTH_STATIC	14
#define IDC_TEXTRES_DEPTH_SLIDER	15

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext);

HRESULT ReinitWindowSizeDependentRenderTargets(ID3D11Device* pd3dDevice);


void InitApp();
void RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackMouse(OnMouseEvent, true);

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Volumetric Diffusion" );
	DXUTCreateDevice (D3D_FEATURE_LEVEL_11_0, true, g_Width, g_Height);
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );
	
	WCHAR sz[100];

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
	g_SampleUI.AddButton( IDC_CHANGE_CONTROL, L"Change contr. surface", 0, iY, 170, 30);
	g_SampleUI.AddRadioButton( IDC_ROTATE, IDC_ROTATE_MOVE_CAMERA, L"Rotate & Scale", 0, iY += 40, 170, 30);
	g_SampleUI.AddRadioButton( IDC_MOVE, IDC_ROTATE_MOVE_CAMERA, L"Move", 0, iY += 26, 170, 30);
	g_SampleUI.AddRadioButton( IDC_CAMERA, IDC_ROTATE_MOVE_CAMERA, L"Camera", 0, iY += 26, 170, 30);
	g_SampleUI.GetRadioButton( IDC_ROTATE )->SetChecked(true);
	g_SampleUI.AddButton( IDC_CHANGE_TEXTRES, L"Change texture res.", 0, iY+=52, 170, 30);
	StringCchPrintf( sz, 100, L"Texture Width: %d", g_iTextureWidth ); 
	g_SampleUI.AddStatic( IDC_TEXTRES_WIDTH_STATIC, sz, 15, iY += 35, 125, 22 );
	g_SampleUI.AddSlider( IDC_TEXTRES_WIDTH_SLIDER, 15, iY += 20, 130, 22, 100, 300, 200 );

    StringCchPrintf( sz, 100, L"Texture Height: %d", g_iTextureHeight ); 
    g_SampleUI.AddStatic( IDC_TEXTRES_HEIGHT_STATIC, sz, 15, iY += 24, 125, 22 );
    g_SampleUI.AddSlider( IDC_TEXTRES_HEIGHT_SLIDER, 15, iY += 20, 130, 22, 100, 300, 200);

    StringCchPrintf( sz, 100, L"Texture Depth: %d", g_iTextureDepth ); 
    g_SampleUI.AddStatic( IDC_TEXTRES_DEPTH_STATIC, sz, 15, iY += 24, 125, 22 );
    g_SampleUI.AddSlider( IDC_TEXTRES_DEPTH_SLIDER, 15, iY += 20, 130, 22, 100, 300, 200);

	// Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -4.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Uncomment this to get debug information from D3D11
    //pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	g_fElapsedTime = fElapsedTime;
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	g_pTxtHelper->End();//important for SAFE_DELETE

}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
	if(g_bCameraActive)
		g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
        }
    }
}

//--------------------------------------------------------------------------------------
// Handle mouse
//--------------------------------------------------------------------------------------
void CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext)
{
	if(g_bCameraActive)
		return;

	if(!bLeftDown)
		g_bBlockMouseDragging = false;

	if(g_bBlockMouseDragging)
		return;
	
	if(g_mouseX == 0 && g_mouseY == 0)
	{
		g_mouseX = iX;
		g_mouseY = iY;
	}


	if(g_bRotatesWithMouse)//Rotate&Scale object
	{
		if(bLeftDown)
		{
			g_pScene->RotateX((g_mouseY-iY)*g_fElapsedTime*g_mouseSpeed);
			g_pScene->RotateY((g_mouseX-iX)*g_fElapsedTime*g_mouseSpeed);
		}
		
		if(iWheelDelta>0)
			g_pScene->Scale(1.0+g_fElapsedTime*100);
		else if(iWheelDelta<0)
			g_pScene->Scale(1.0-g_fElapsedTime*100);
	}
	else//Move object
	{
		const D3DXMATRIX* mView = g_Camera.GetViewMatrix();
		D3DXVECTOR3 lookAt = D3DXVECTOR3(mView->_13, mView->_23,mView->_33);
		D3DXVECTOR3 lookRight = D3DXVECTOR3(mView->_11, mView->_21,mView->_31);
		D3DXVECTOR3 lookUp = D3DXVECTOR3(mView->_12, mView->_22,mView->_32);

		if(bLeftDown)
		{
			g_pScene->Translate(g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.x, g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.y, g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.z);
			g_pScene->Translate(g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.x, g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.y, g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.z);
		}

		if(iWheelDelta>0)
			g_pScene->Translate(100*g_fElapsedTime*lookAt.x, 100*g_fElapsedTime*lookAt.y, 100*g_fElapsedTime*lookAt.z);
		else if(iWheelDelta<0)
			g_pScene->Translate(-100*g_fElapsedTime*lookAt.x, -100*g_fElapsedTime*lookAt.y, -100*g_fElapsedTime*lookAt.z);
	}

	g_mouseX = iX;
	g_mouseY = iY;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); 
			break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); 
			break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); 
			break;
		
		// Custom app controls
		case IDC_CHANGE_CONTROL:
			g_pScene->ChangeControlledSurface();
			break;
		case IDC_ROTATE:
			g_bRotatesWithMouse = true;
			g_bCameraActive = false;
			break;
		case IDC_MOVE:
			g_bRotatesWithMouse = false;
			g_bCameraActive = false;
			break;
		case IDC_CAMERA:
			g_bCameraActive = true;
			break;
		case IDC_TEXTRES_WIDTH_SLIDER:
			g_iTextureWidth = g_SampleUI.GetSlider(IDC_TEXTRES_WIDTH_SLIDER)->GetValue();
			WCHAR sz[100];
			StringCchPrintf( sz, 100, L"Texture Width: %d", g_iTextureWidth ); 
            g_SampleUI.GetStatic( IDC_TEXTRES_WIDTH_STATIC )->SetText( sz );
			g_bBlockMouseDragging = true;
			break;
		case IDC_TEXTRES_HEIGHT_SLIDER:
			g_iTextureHeight = g_SampleUI.GetSlider(IDC_TEXTRES_HEIGHT_SLIDER)->GetValue();
			StringCchPrintf( sz, 100, L"Texture Height: %d", g_iTextureHeight ); 
            g_SampleUI.GetStatic( IDC_TEXTRES_HEIGHT_STATIC )->SetText( sz );
			g_bBlockMouseDragging = true;
			break;
		case IDC_TEXTRES_DEPTH_SLIDER:
			g_iTextureDepth = g_SampleUI.GetSlider(IDC_TEXTRES_DEPTH_SLIDER)->GetValue();
			StringCchPrintf( sz, 100, L"Texture Depth: %d", g_iTextureDepth ); 
            g_SampleUI.GetStatic( IDC_TEXTRES_DEPTH_STATIC )->SetText( sz );
			g_bBlockMouseDragging = true;
			break;
    }

}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}




//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
	g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );
    
	g_pScene = new Scene(pd3dDevice, pd3dImmediateContext);
	V_RETURN(g_pScene->Initialize(g_iTextureWidth, g_iTextureHeight, g_iTextureDepth));
	V_RETURN(g_pScene->SetScreenSize(g_Width, g_Height));

	// Initialize the view matrix
    g_Camera.SetViewParams( &g_Eye, &g_At );
    g_Camera.SetEnablePositionMovement(true);
    g_Camera.SetScalers(0.004f, 20.0f);
    g_View = *g_Camera.GetViewMatrix();
    g_Projection = *g_Camera.GetProjMatrix();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 2.0f, 4000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, 100 );
    g_SampleUI.SetSize( 170, 300 );

	V_RETURN(ReinitWindowSizeDependentRenderTargets(pd3dDevice));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Initialize any textures that must match the window size
//--------------------------------------------------------------------------------------
HRESULT ReinitWindowSizeDependentRenderTargets(ID3D11Device* pd3dDevice)
{
    HRESULT hr;

    // Create resources to enable writing the scene depth using MRT, as well as to 
    //  enable reading as a shader resource
    ID3D11RenderTargetView *pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11Resource *pRTVResource;
    pRTV->GetResource(&pRTVResource);
    ID3D11Texture2D *pRTVTex2D = static_cast<ID3D11Texture2D*>(pRTVResource);
    assert(pRTVTex2D);
    D3D11_TEXTURE2D_DESC pRTVTex2DDesc;
    pRTVTex2D->GetDesc(&pRTVTex2DDesc);
    pRTVResource->Release();    

    SAFE_RELEASE(g_pSceneDepthTex2DNonMS);
    SAFE_RELEASE(g_pSceneDepthTex2D);
    SAFE_RELEASE(g_pSceneDepthRTV);
    SAFE_RELEASE(g_pSceneDepthSRV);

    D3D11_TEXTURE2D_DESC desc;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MipLevels = 1;
    desc.MiscFlags = 0;
    desc.SampleDesc = pRTVTex2DDesc.SampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Width = g_Width;
    desc.Height = g_Height;
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    V_RETURN(pd3dDevice->CreateTexture2D(&desc,NULL,&g_pSceneDepthTex2D));

    // We need a Non-Multisampled texture2D resource of the same dimensions to read from in shaders
    if(pRTVTex2DDesc.SampleDesc.Count > 1)
    {
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        V_RETURN(pd3dDevice->CreateTexture2D(&desc,NULL,&g_pSceneDepthTex2DNonMS));
    }
    else
    {
        g_pSceneDepthTex2DNonMS = g_pSceneDepthTex2D;
        g_pSceneDepthTex2DNonMS->AddRef();
    }

    // Create the render target view for the potentially Multisampled texture2D resource
    D3D11_RENDER_TARGET_VIEW_DESC descRTV;
    descRTV.Format = DXGI_FORMAT_R32_FLOAT;
    if( pRTVTex2DDesc.SampleDesc.Count <= 1 )
    {
        descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        descRTV.Texture2D.MipSlice = 0;
    }
    else
    {
        descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    }
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pSceneDepthTex2D, &descRTV, &g_pSceneDepthRTV ) );

    // Create a shader resource view for a Non-MS texture
    D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
    descSRV.Format = DXGI_FORMAT_R32_FLOAT;
    descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    descSRV.Texture2D.MipLevels = 1;
    descSRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView(g_pSceneDepthTex2DNonMS, &descSRV, &g_pSceneDepthSRV) );

    return S_OK;

}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
		
    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	//extreme performance loss:
	//float colorZ[4] = { g_zFar,0,0,0};
	//pd3dImmediateContext->ClearRenderTargetView( g_pSceneDepthRTV, colorZ );
	
	// Create a viewport to match the screen size
    D3D11_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;
    rtViewport.Width = g_Width;
    rtViewport.Height = g_Height;

    // If the settings dialog is being shown, then
    //  render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        // Set the viewport
        pd3dImmediateContext->RSSetViewports(1,&rtViewport);
        // Render the scene to the screen
        pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV , pDSV ); 
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        pd3dImmediateContext->OMSetRenderTargets( 0, NULL, NULL );
        return;
    }

	// Initialize the world matrices for the simulation Grid and the obstacle Mesh
    //   Grid
    D3DXMATRIX gridScale, gridRotate;
    D3DXMatrixScaling(&gridScale, 20.0f, 20.0f, 20.0f );
    D3DXMatrixRotationX(&gridRotate, 3.0f*3.1416f/2.0f);
    g_gridWorld = gridScale * gridRotate;

	// UPDATE GLOBAL VARIABLES FOR VOLUME RENDERING
	g_View = *g_Camera.GetViewMatrix();
	g_Projection = *g_Camera.GetProjMatrix();
	
	D3DXMATRIX mViewProjection;
    D3DXMATRIX mProj = *g_Camera.GetProjMatrix();
    D3DXMATRIX mView = *g_Camera.GetViewMatrix();

	mViewProjection = mView * mProj;

	g_pScene->Render(mViewProjection);

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE(g_pTxtHelper);

    SAFE_DELETE(g_pScene);
}