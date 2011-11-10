// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )
// Include the D3D10 headers
//--------------------------
#include <d3d10.h>
#include <d3dx10.h>
#include <d3dx9.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "Surface.h"


Surface::Surface()
{
}


Surface::~Surface()
{
}

bool stringStartsWith(const char *s, const char *val)
{
        return !strncmp(s, val, strlen(val));
}

void Surface::ReadVectorFile(char *s)
{
	char buff[256];
	WCHAR wcFileInfo[512];
	char *token;

	FILE *F = fopen(s, "rb");

	FILE *F_out = fopen("Media\\test_ReadVectorFile.txt", "w");
	char c[256];

	while (fgets(buff, 255, F))
		if (stringStartsWith(buff, "<!DOCTYPE SurfaceXML"))
		{
			fputs ("(INFO) : This seems to be a diffusion surface file.\n", F_out);//test
			break;
		}
	fgets(buff, 255, F);
	token = strtok(buff, " \"\t");
	while (!stringStartsWith(token, "nb_control_points="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_pNum = atof(token);
	sprintf(c, "m_pNum: %d \n", m_pNum);//test
	fputs(c, F_out);//test
	while (!stringStartsWith(token, "nb_left_colors="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_clNum = atof(token);
	sprintf(c, "m_clNum: %d \n", m_clNum);//test
	fputs(c, F_out);//test
	while (!stringStartsWith(token, "nb_right_colors="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_crNum = atof(token);
	sprintf(c, "m_crNum: %d \n", m_crNum);//test
	fputs(c, F_out);//test
	while (!stringStartsWith(token, "nb_blur_points="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_bNum = atof(token);
	sprintf(c, "m_bNum: %d \n", m_bNum);//test
	fputs(c, F_out);//test
	
	D3DXVECTOR3 maxBound = D3DXVECTOR3(-1000000,-1000000,-1000000);
	D3DXVECTOR3 minBound = D3DXVECTOR3(1000000,1000000,1000000);
	
	m_controlpoints = new D3DXVECTOR3[m_pNum];
	for(int i=0; i < m_pNum; i++)
	{
		while(!stringStartsWith(buff, "  <control_point "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "x="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_controlpoints[i].x = atof(token);
		while (!stringStartsWith(token, "y="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_controlpoints[i].y = atof(token);
		while (!stringStartsWith(token, "z="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_controlpoints[i].z = atof(token);
		fgets(buff, 255, F);
		
		sprintf(c, "controlpoint[%d]=(%g,%g,%g) \n", i, m_controlpoints[i].x, m_controlpoints[i].y, m_controlpoints[i].z);
		fputs(c, F_out);

		//extend the bounds if necessary
			if (m_controlpoints[i].y < minBound.y)
				minBound.y = m_controlpoints[i].y;
			if (m_controlpoints[i].y > maxBound.y)
				maxBound.y = m_controlpoints[i].y;
			if (m_controlpoints[i].x < minBound.x)
				minBound.x = m_controlpoints[i].x;
			if (m_controlpoints[i].x > maxBound.x)
				maxBound.x = m_controlpoints[i].x;
			if (m_controlpoints[i].z < minBound.z)
				minBound.z = m_controlpoints[i].z;
			if (m_controlpoints[i].z > maxBound.z)
				maxBound.z = m_controlpoints[i].z;
	}
	
	m_colors_left = new COLORPOINT[m_clNum];
	for (int i = 0; i < m_clNum; i++)
	{
		while (!stringStartsWith(buff, "  <left_color "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "G="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_left[i].col.y = atof(token)/256.0;
		
		while (!stringStartsWith(token, "R="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_left[i].col.z = atof(token)/256.0;
		
		while (!stringStartsWith(token, "globalID="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_left[i].off = atof(token);

		while (!stringStartsWith(token, "B="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_left[i].col.x = atof(token)/256.0;
		fgets(buff, 255, F);

		sprintf(c, "color_left[%d]=(%g,%g,%g,%d) \n", i, m_colors_left[i].col.x, m_colors_left[i].col.y, m_colors_left[i].col.z, m_colors_left[i].off);
		fputs(c, F_out);
	}
	
	m_colors_right = new COLORPOINT[m_crNum];
	m_colors_left = new COLORPOINT[m_clNum];
	for (int i = 0; i < m_clNum; i++)
	{
		while (!stringStartsWith(buff, "  <right_color "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "G="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_right[i].col.y = atof(token)/256.0;
		
		while (!stringStartsWith(token, "R="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_right[i].col.z = atof(token)/256.0;
		
		while (!stringStartsWith(token, "globalID="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_right[i].off = atof(token);

		while (!stringStartsWith(token, "B="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_colors_right[i].col.x = atof(token)/256.0;
		fgets(buff, 255, F);

		sprintf(c, "color_right[%d]=(%g,%g,%g,%d) \n", i, m_colors_right[i].col.x, m_colors_right[i].col.y, m_colors_right[i].col.z, m_colors_right[i].off);
		fputs(c, F_out);
	}

	m_blurrpoints = new BLURRPOINT[m_bNum];
	for (int i = 0; i < m_bNum; i++)
	{
		while (!stringStartsWith(buff, "  <best_scale"))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "value="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_blurrpoints[i].blurr = atof(token);

		while (!stringStartsWith(token, "globalID="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_blurrpoints[i].off = atof(token);
		fgets(buff, 255, F);

		sprintf(c, "blurrpoint[%d]=(%f,%d) \n", i, m_blurrpoints[i].blurr, m_blurrpoints[i].off);
		fputs(c, F_out);
	}
	fclose(F);
	fclose(F_out);

	//scale the whole image between -1 and 1
	/*D3DXVECTOR2 middlePan = D3DXVECTOR2( 0.5*(maxBound.x+minBound.x), 0.5*(maxBound.y+minBound.y));
	for (int i1=0; i1<m_cNum; i1++)
		for (int i2=0; i2<m_curve[i1].pNum; i2++)
		{
			m_curve[i1].p[i2].x = 2.0f*(m_curve[i1].p[i2].x-middlePan.x)/m_fWidth;
			m_curve[i1].p[i2].y = 2.0f*(m_curve[i1].p[i2].y-middlePan.y)/m_fHeight;
		}
	StringCchPrintf( wcFileInfo, 512, L"(INFO) : %d curve segments found in file.\n", m_cSegNum);
	OutputDebugString( wcFileInfo );
	*/
}