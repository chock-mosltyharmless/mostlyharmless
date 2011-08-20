#pragma once

#define CAM_WIDTH 640
#define CAM_HEIGHT 480 // 360 for product...
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 512
#define TEXTURE_U_RANGE ((float)CAM_WIDTH / (float)TEXTURE_WIDTH)
#define TEXTURE_V_RANGE ((float)CAM_HEIGHT / (float)TEXTURE_HEIGHT)

class VideoTexture
{
public:
	VideoTexture(void);
	~VideoTexture(void);

public:
	HRESULT init();
	void deinit();
	void captureFrame();
	void drawScreenSpaceQuad(float startX, float startY, float endX, float endY);

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

private:
	IGraphBuilder *pGraph;
	ICaptureGraphBuilder2 *pBuild;
	IMediaControl *pMediaControl;
	IMediaEvent *pMediaEvent;
	ISampleGrabber *pGrabber;
	IBaseFilter *pGrabberBase;
	IBaseFilter *pNullRender;
	GLuint texture;
};

