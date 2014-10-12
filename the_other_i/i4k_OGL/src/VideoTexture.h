#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
// Windows-Headerdateien:
#include <windows.h>

// C RunTime-Headerdateien
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>

#include <dshow.h>
#include <Dvdmedia.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "qedit.h"


#define CAM_WIDTH 640
#define CAM_HEIGHT 480 // 360 for product...
//#define TEXTURE_WIDTH 1024
//#define TEXTURE_HEIGHT 512
#define TEXTURE_WIDTH 640
#define TEXTURE_HEIGHT 480
#define TEXTURE_U_RANGE ((float)CAM_WIDTH / (float)TEXTURE_WIDTH)
#define TEXTURE_V_RANGE ((float)CAM_HEIGHT / (float)TEXTURE_HEIGHT)

class VideoTexture
{
public:
	VideoTexture(void);
	~VideoTexture(void);

public:
	// use cameraID 1 for first and so on
	HRESULT init(int cameraID);
	void deinit();
	void captureFrame();

private:
	HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
	HRESULT DisconnectPins(IBaseFilter *pFilter);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);
	void LocalFreeMediaType(AM_MEDIA_TYPE& mt);
	HRESULT InitCaptureGraphBuilder(IGraphBuilder **ppGraph, ICaptureGraphBuilder2 **ppBuild);
	HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum);
	void DisplayDeviceInformation(IEnumMoniker *pEnum);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest);
	void LocalDeleteMediaType(AM_MEDIA_TYPE *pmt);

public:
	// Note: This data is only usable after a call to captureFrame()
	unsigned char textureData[CAM_WIDTH*CAM_HEIGHT*3];	
	GLuint textures[1]; // 1: fg

private:
	IGraphBuilder *pGraph;
	ICaptureGraphBuilder2 *pBuild;
	IMediaControl *pMediaControl;
	IMediaEvent *pMediaEvent;
	ISampleGrabber *pGrabber;
	IBaseFilter *pGrabberBase;
	IBaseFilter *pNullRender;	
};

