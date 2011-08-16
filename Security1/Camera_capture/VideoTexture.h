#pragma once

#define CAM_WIDTH 640
#define CAM_HEIGHT 360

class VideoTexture
{
public:
	VideoTexture(void);
	~VideoTexture(void);

public:
	HRESULT init();
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

private:
	unsigned char *textureData[CAM_WIDTH*CAM_HEIGHT*3];
	IGraphBuilder *pGraph;
	ICaptureGraphBuilder2 *pBuild;
	IMediaControl *pMediaControl;
	IMediaEvent *pMediaEvent;
	ISampleGrabber *pGrabber;
	IBaseFilter *pGrabberBase;
	IBaseFilter *pNullRender;
	GLuint texture;
};

