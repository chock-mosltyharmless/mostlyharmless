#include "StdAfx.h"
#include "VideoTexture.h"

VideoTexture::VideoTexture(void) :
	pGraph(NULL),
    pBuild(NULL),
	pMediaControl(NULL),
	pMediaEvent(NULL),
	pGrabber(NULL),
	pGrabberBase(NULL),
	pNullRender(NULL),
	texture(-1)

{
}

VideoTexture::~VideoTexture(void)
{
}

// Get frame from camera and set the texture.
void VideoTexture::captureFrame()
{
	// bind texture
	glBindTexture(GL_TEXTURE_2D, texture);

	// capture from camera:
	long pBufferSize = CAM_WIDTH*CAM_HEIGHT*3;
	pGrabber->GetCurrentBuffer(&pBufferSize, (long *)textureData);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CAM_WIDTH, CAM_HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, textureData);
}

HRESULT VideoTexture::init()
{
	// Texture -> This will be put into the camera module	
	glGenTextures(1, &texture);					// Create The Texture
	// Typical Texture Generation Using Data From The Bitmap
	glBindTexture(GL_TEXTURE_2D, texture);
	// Generate The Texture (640x480... make changeable!)
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, ...THe data111!!!);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glTexImage2D(GL_TEXTURE_2D, 0, 3, CAM_WIDTH, CAM_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

	// Video stuff:
	// Create captue graph builder:
	HRESULT hr = InitCaptureGraphBuilder(&pGraph, &pBuild);
	if (FAILED(hr)) return hr;
	IEnumMoniker *enumerator;
	hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &enumerator);
	//DisplayDeviceInformation(enumerator);
	// Take the first camera:
	IMoniker *pMoniker = NULL;
	enumerator->Next(1, &pMoniker, NULL);
	IBaseFilter *pCap = NULL;
	hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCap);
	if (SUCCEEDED(hr))
	{
		hr = pGraph->AddFilter(pCap, L"Capture Filter");
		if (FAILED(hr)) return hr;
	}
	else return hr;

	// Create the Sample Grabber which we will use
	// To take each frame for texture generation
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
							IID_ISampleGrabber, (void **)&pGrabber);
	if (FAILED(hr)) return hr;
	hr = pGrabber->QueryInterface(IID_IBaseFilter, (void **)&pGrabberBase);
		// We have to set the 24-bit RGB desire here
	// So that the proper conversion filters
	// Are added automatically.
	AM_MEDIA_TYPE desiredType;
	memset(&desiredType, 0, sizeof(desiredType));
	desiredType.majortype = MEDIATYPE_Video;
	desiredType.subtype = MEDIASUBTYPE_RGB24;
	desiredType.formattype = FORMAT_VideoInfo;
	pGrabber->SetMediaType(&desiredType);
	pGrabber->SetBufferSamples(TRUE);
	// add to Graph
	pGraph->AddFilter(pGrabberBase, L"Grabber");

    /* Null render filter */
    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pNullRender);
    if(FAILED(hr)) return hr;
	pGraph->AddFilter(pNullRender, L"Render");

	// Connect the graph
    hr = ConnectFilters(pGraph, pCap, pGrabberBase); 
    if(FAILED(hr)) return hr;
	hr = ConnectFilters(pGraph, pGrabberBase, pNullRender);

	// Set output format of capture:
	IAMStreamConfig *pConfig = NULL;
    hr = pBuild->FindInterface(
                &PIN_CATEGORY_CAPTURE, // Capture pin.
                0,    // Any media type.
                pCap, // Pointer to the capture filter.
                IID_IAMStreamConfig, (void**)&pConfig);
	if (FAILED(hr)) return hr;
	AM_MEDIA_TYPE *pmtConfig;
	hr = pConfig->GetFormat(&pmtConfig);
	if (FAILED(hr)) return hr;
		
	// Try and find a good video format
    int iCount = 0, iSize = 0;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);               
    // Check the size to make sure we pass in the correct structure.
    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
		// Use the video capabilities structure.               
        for (int iFormat = 0; iFormat < iCount; iFormat++)
        {
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE *pmtConfig;
			hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
			if (SUCCEEDED(hr))
			{
				VIDEOINFOHEADER *hdr = (VIDEOINFOHEADER *)pmtConfig->pbFormat;
				if (hdr->bmiHeader.biWidth == CAM_WIDTH &&
					hdr->bmiHeader.biHeight == CAM_HEIGHT &&
					hdr->bmiHeader.biBitCount == 24)
				{
					pConfig->SetFormat(pmtConfig);
				}
			}
		}
	}
	pConfig->Release();

	// Set camera stuff
	IAMCameraControl *pCamControl = NULL;
	hr = pCap->QueryInterface(IID_IAMCameraControl, (void **)&pCamControl);
	if (FAILED(hr)) return hr;
	// Get the range and default value. 
	long Min, Max, Step, Default, Flags;
	// For getting: long Val;
	hr = pCamControl->GetRange(CameraControl_Focus, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pCamControl->Set(CameraControl_Focus, 0, CameraControl_Flags_Manual);
	hr = pCamControl->GetRange(CameraControl_Exposure, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pCamControl->Set(CameraControl_Exposure, -4, CameraControl_Flags_Manual);
	pCamControl->Release();
	IAMVideoProcAmp *pProcAmp = 0;
	hr = pCap->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
	if (FAILED(hr)) return hr;
	hr = pProcAmp->GetRange(VideoProcAmp_Brightness, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pProcAmp->Set(VideoProcAmp_Brightness, 30, VideoProcAmp_Flags_Manual);
	hr = pProcAmp->GetRange(VideoProcAmp_Gain, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pProcAmp->Set(VideoProcAmp_Gain, 30, VideoProcAmp_Flags_Manual);
	hr = pProcAmp->GetRange(VideoProcAmp_WhiteBalance, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pProcAmp->Set(VideoProcAmp_WhiteBalance, 4500, VideoProcAmp_Flags_Manual);
	hr = pProcAmp->GetRange(VideoProcAmp_Saturation, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pProcAmp->Set(VideoProcAmp_Saturation, 100, VideoProcAmp_Flags_Manual);		
	hr = pProcAmp->GetRange(VideoProcAmp_Contrast, &Min, &Max, &Step, &Default, &Flags);
	if (SUCCEEDED(hr)) pProcAmp->Set(VideoProcAmp_Contrast, 6, VideoProcAmp_Flags_Manual);		
	pProcAmp->Release();

	hr = pMediaControl->Run();
	return hr;
}

void VideoTexture::deinit()
{
	// Delete texture
	if (texture >= 0) glDeleteTextures(1, &texture); texture = -1;

	// Release video stuff
	if (pMediaControl) pMediaControl->Stop();
	if (pMediaControl) pMediaControl->Release(); pMediaControl = NULL;
	if (pMediaEvent) pMediaEvent->Release(); pMediaEvent = NULL;
	if (pGrabber) pGrabber->Release(); pGrabber = NULL;
	if (pGrabberBase) pGrabberBase->Release(); pGrabberBase = NULL;
	if (pNullRender) pNullRender->Release(); pNullRender = NULL;
	if (pGraph) pGraph->Release(); pGraph = NULL;
	if (pBuild) pBuild->Release(); pBuild = NULL;
}

// GetUnconnectedPin   
//    Finds an unconnected pin on a filter in the desired direction   
HRESULT VideoTexture::GetUnconnectedPin(   
						  IBaseFilter *pFilter,   // Pointer to the filter.   
						  PIN_DIRECTION PinDir,   // Direction of the pin to find.   
						  IPin **ppPin)           // Receives a pointer to the pin.   
{   
	*ppPin = 0;   
	IEnumPins *pEnum = 0;   
	IPin *pPin = 0;   
	HRESULT hr = pFilter->EnumPins(&pEnum);   
	if (FAILED(hr))   
	{   
		return hr;   
	}   
	while (pEnum->Next(1, &pPin, NULL) == S_OK)   
	{   
		PIN_DIRECTION ThisPinDir;   
		pPin->QueryDirection(&ThisPinDir);   
		if (ThisPinDir == PinDir)   
		{   
			IPin *pTmp = 0;   
			hr = pPin->ConnectedTo(&pTmp);   
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.   
			{   
				pTmp->Release();   
			}   
			else  // Unconnected, this is the pin we want.   
			{   
				pEnum->Release();   
				*ppPin = pPin;   
				return S_OK;   
			}   
		}   
		pPin->Release();   
	}   
	pEnum->Release();   
	// Did not find a matching pin.   
	return E_FAIL;   
}   
 
// Disconnect any connections to the filter.   
HRESULT VideoTexture::DisconnectPins(IBaseFilter *pFilter)
{   
	IEnumPins *pEnum = 0;   
	IPin *pPin = 0;   
	HRESULT hr = pFilter->EnumPins(&pEnum);   
	if (FAILED(hr))   
	{   
		return hr;   
	}   
 
	while (pEnum->Next(1, &pPin, NULL) == S_OK)   
	{   
		pPin->Disconnect();   
		pPin->Release();   
	}   
	pEnum->Release();   
 
	// Did not find a matching pin.   
	return S_OK;   
}   

// ConnectFilters   
//    Connects two filters   
HRESULT VideoTexture::ConnectFilters(   
					   IGraphBuilder *pGraph,    
					   IBaseFilter *pSrc,    
					   IBaseFilter *pDest)   
{   
	if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))   
	{   
		return E_POINTER;   
	}   
 
	// Find an output pin on the first filter.   
	IPin *pOut = 0;   
	HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);   
	if (FAILED(hr))    
	{   
		return hr;   
	}   
	hr = ConnectFilters(pGraph, pOut, pDest);   
	pOut->Release();
	return hr;   
}   
 
// LocalFreeMediaType   
//    Free the format buffer in the media type   
void VideoTexture::LocalFreeMediaType(AM_MEDIA_TYPE& mt)
{   
	if (mt.cbFormat != 0)   
	{   
		CoTaskMemFree((PVOID)mt.pbFormat);   
		mt.cbFormat = 0;   
		mt.pbFormat = NULL;   
	}   
	if (mt.pUnk != NULL)   
	{   
		// Unecessary because pUnk should not be used, but safest.   
		mt.pUnk->Release();   
		mt.pUnk = NULL;   
	}   
}   

HRESULT VideoTexture::InitCaptureGraphBuilder(
  IGraphBuilder **ppGraph,  // Receives the pointer.
  ICaptureGraphBuilder2 **ppBuild  // Receives the pointer.
)
{
    if (!ppGraph || !ppBuild)
    {
        return E_POINTER;
    }
    IGraphBuilder *pGraph = NULL;
    ICaptureGraphBuilder2 *pBuild = NULL;

    // Create the Capture Graph Builder.
    HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, 
        CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuild );
    if (SUCCEEDED(hr))
    {
        // Create the Filter Graph Manager.
        hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void**)&pGraph);
        if (SUCCEEDED(hr))
        {
            // Initialize the Capture Graph Builder.
            pBuild->SetFiltergraph(pGraph);

            // Return both interface pointers to the caller.
            *ppBuild = pBuild;
            *ppGraph = pGraph; // The caller must release both interfaces.

			// media control and so on
			hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
			if (FAILED(hr)) return hr;
			hr = pGraph->QueryInterface (IID_IMediaEvent, (void **)&pMediaEvent);
			if (FAILED(hr)) return hr;

            return S_OK;
        }
        else
        {
            pBuild->Release();
        }
    }
    return hr; // Failed
}

HRESULT VideoTexture::EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,  
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void VideoTexture::DisplayDeviceInformation(IEnumMoniker *pEnum)
{
    IMoniker *pMoniker = NULL;

    while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;  
        } 

        VARIANT var;
        VariantInit(&var);

        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, 0);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
        }
        if (SUCCEEDED(hr))
        {
            printf("%S\n", var.bstrVal);
            VariantClear(&var); 
        }

        hr = pPropBag->Write(L"FriendlyName", &var);

        // WaveInID applies only to audio capture devices.
        hr = pPropBag->Read(L"WaveInID", &var, 0);
        if (SUCCEEDED(hr))
        {
            printf("WaveIn ID: %d\n", var.lVal);
            VariantClear(&var); 
        }

        hr = pPropBag->Read(L"DevicePath", &var, 0);
        if (SUCCEEDED(hr))
        {
            // The device path is not intended for display.
            printf("Device path: %S\n", var.bstrVal);
            VariantClear(&var); 
        }

        pPropBag->Release();
        pMoniker->Release();
    }
}

// ConnectFilters   
//    Connects a pin of an upstream filter to the pDest downstream filter   
HRESULT VideoTexture::ConnectFilters(   
					   IGraphBuilder *pGraph, // Filter Graph Manager.   
					   IPin *pOut,            // Output pin on the upstream filter.   
					   IBaseFilter *pDest)    // Downstream filter.   
{   
	if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))   
	{   
		return E_POINTER;   
	}   
#ifdef debug   
	PIN_DIRECTION PinDir;   
	pOut->QueryDirection(&PinDir);   
	_ASSERTE(PinDir == PINDIR_OUTPUT);   
#endif   
 
	// Find an input pin on the downstream filter.   
	IPin *pIn = 0;   
	HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);   
	if (FAILED(hr))   
	{   
		return hr;   
	}   
	// Try to connect them.   
	hr = pGraph->Connect(pOut, pIn);   
	pIn->Release();   
	return hr;   
}   

// LocalDeleteMediaType   
//    Free the format buffer in the media type,    
//    then delete the MediaType ptr itself   
void VideoTexture::LocalDeleteMediaType(AM_MEDIA_TYPE *pmt)
{   
	if (pmt != NULL)   
	{   
		LocalFreeMediaType(*pmt); // See FreeMediaType for the implementation.   
		CoTaskMemFree(pmt);   
	}   
}
