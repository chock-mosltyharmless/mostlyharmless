#pragma once

#define CAM_WIDTH 640
#define CAM_HEIGHT 480 // 360 for product...
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 512
#define TEXTURE_U_RANGE ((float)CAM_WIDTH / (float)TEXTURE_WIDTH)
#define TEXTURE_V_RANGE ((float)CAM_HEIGHT / (float)TEXTURE_HEIGHT)

// Face constants, in downsampled space!
#define MAX_NUM_FACES 200
#define FACE_WIDTH 16
#define FACE_HEIGHT 20
#define FACE_X_SHIFT 30
#define FACE_Y_SHIFT 30

class VideoTexture
{
public:
	VideoTexture(void);
	~VideoTexture(void);

public:
	// use cameraID 1 for first and so on
	HRESULT init(int cameraID);
	void deinit();
	void captureFrame(int additionalUpdate);
	void drawScreenSpaceQuad(float startX, float startY, float endX, float endY);
	void addFace(float xPos, float yPos, int specialFace);  // range 0..1 for each!
	int getNearestFace(float xPos, float yPos);
	void setFaceRect(int faceID, float xPos, float yPos);
	void drawFaceBoxes(float startX, float startY, float endX, float endY, int time,
					   bool onlyFaceThree, int highlight = -1);
	void updateFaceBoxes(bool onlyFaceThree);
	void getFaceRect(int faceID, float *dest,
					 float startX, float startY, float endX, float endY);
	void removeLastFace();

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
	void drawBox(float startX, float startY, float endX, float endY);

public:
	// Note: This data is only usable after a call to captureFrame()
	unsigned char textureData[CAM_WIDTH*CAM_HEIGHT*3];	
	int boxFaceSpecial[3]; // up to three faces are special (red)
	GLuint textures[3]; // 1: fg, 2: bg

private:
	IGraphBuilder *pGraph;
	ICaptureGraphBuilder2 *pBuild;
	IMediaControl *pMediaControl;
	IMediaEvent *pMediaEvent;
	ISampleGrabber *pGrabber;
	IBaseFilter *pGrabberBase;
	IBaseFilter *pNullRender;
	
	// face recognition stuff
	int numFaces;
	unsigned char faceData[MAX_NUM_FACES][FACE_HEIGHT][FACE_WIDTH][3];
	// In texture space!
	int faceLocation[MAX_NUM_FACES][2];
	bool drawFaceBox[MAX_NUM_FACES];
	int currentFaceLocation[MAX_NUM_FACES][2];
};

