// voice1.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "voice1.h"
#include "fft/FFTRealFixLen.h"
#include <stdio.h>
#include <mmsystem.h>

#define MAX_LOADSTRING 100
#define LOG_WINDOW_SIZE 11
#define WINDOW_SIZE (1<<LOG_WINDOW_SIZE)
#define FRAME_STEP (WINDOW_SIZE/4)
#define SYRES 400
#define SXRES 1000 
// The number of "mel-" bands
#define NUM_BANDS 12
#define NOISE_LENGTH (1<<20)
#define PITCH_RESOLUTION 24

// Globale Variablen:
HINSTANCE hInst;								// Aktuelle Instanz
HWND hWnd;
TCHAR szTitle[MAX_LOADSTRING];					// Titelleistentext
TCHAR szWindowClass[MAX_LOADSTRING];			// Klassenname des Hauptfensters
// In this variable I store what to render. 0: ref, 1: wave, 2:difference
static int renderObject = 0;
static const BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER),SXRES,-SYRES,1,32,BI_RGB,0,0,0,0,0},{0,0,0,0}};
static unsigned int buffer[SXRES*SYRES];
static double hannWindow[WINDOW_SIZE];
// temporary buffer for the samples of the currently done window
static double windowBuffer[WINDOW_SIZE];
// temporary buffer for the reference fft;
static double *refSpec = 0;
static int refSpecNumFrames = 0;
static double *spec = 0;
// template for a wave file
#define MZK_NUMCHANNELS 1
#define MZK_RATE 44100
static int wavHeader[11] = {
    0x46464952, 
    0,//MZK_NUMSAMPLESC*sizeof(short)+36, 
    0x45564157, 
    0x20746D66, 
    16, 
    WAVE_FORMAT_PCM|(MZK_NUMCHANNELS<<16), 
    MZK_RATE, 
    MZK_RATE*MZK_NUMCHANNELS*sizeof(short), 
    (MZK_NUMCHANNELS*sizeof(short))|((8*sizeof(short))<<16),
    0x61746164, 
    //MZK_NUMSAMPLES*MZK_NUMCHANNELS*sizeof(short)
	0
    };
// temporary buffer for the reference wave
short *refwavefile = 0;
static double *refWave = 0;
static int refWaveLength = 0;
short *wavefile = 0;
static double *wave = 0;
FFTRealFixLen <LOG_WINDOW_SIZE> fft_object; // 1024-point (2^10) FFT object constructed.
// The encoding data
static bool dirty = false; // Set to true if we need recalculation of spec
static int *pitch = 0; // The pitch in log2/24 domain. 0 == 1 Hz?
static int *amplitude = 0; // The amplitude in log2 domain (that is 1/2??)
static double *d_amplitude = 0;
static bool *amplitudeDefined = 0;
float filterY[5]; // 4 is for output
double noise[NOISE_LENGTH];

unsigned long seed;
int jrand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	return (seed >> 8) % 65535;
}

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
float jfrand()
{
	return (float)(jrand()) * (1.0f/65536.0f);
}

double filter(double input, double f, double p, double q, int highpass)
{
	double t1, t2;              //temporary buffers

	// Filter (in [-1.0...+1.0])
	input -= q * filterY[4];                          //feedback
	t1 = filterY[1];
	filterY[1] = (float)((input + filterY[0]) * p - filterY[1] * f);
	t2 = filterY[2];
	filterY[2] = (float)((filterY[1] + t1) * p - filterY[2] * f);
	t1 = filterY[3];
	filterY[3] = (float)((filterY[2] + t2) * p - filterY[3] * f);
	filterY[4] = (float)((filterY[3] + t1) * p - filterY[4] * f);
	// Here comes the nonlinearity:
	//filterY[4][channel] = filterY[4][channel] - filterY[4][channel] * filterY[4][channel] * filterY[4][channel] * 0.166667f;    //clipping
	filterY[0] = (float)input;

	if (highpass)
	{
		return input - filterY[4];
	}
	else
	{
		return filterY[4];
	}
// Lowpass  output:  b4
// Highpass output:  in - b4;
// Bandpass output:  3.0f * (b3 - b4);
}

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void estimateFrame(int frameID, double fr = -1.0)
{
	// estimate FFT index of the pitch using the first 3 overtones
	double threePitch, topThreePitch, topEnergy;

	// 150 Hz -> 150*WINDOW_SIZE/44100 = 7
	// 900 Hz -> 42

	// from 150 Hz to 900 Hz
	if (fr < 0)
	{
		topEnergy = 0.0;
		//for (threePitch = 9.0; threePitch < 22.5; threePitch++)
		for (threePitch = 25.0; threePitch < 45.5; threePitch++)
		{
			double energy = 0.0;
			for (int k = 1; k <= 3; k++)
			{
				int fftIndex = (int)(floor(threePitch*k/3.0 + 0.5));
				energy += sqrt(pow(2.0, refSpec[frameID*WINDOW_SIZE+fftIndex]));
			}

			if (energy > topEnergy)
			{
				topThreePitch = threePitch;
				topEnergy = energy;
			}
		}
	}
	else
	{
		topThreePitch = fr;
	}

	// convert to pitch saver
	double freq = (topThreePitch / 3.0) * 44100 / WINDOW_SIZE;
	double pitchIncr = freq / 44100.0 * 3.1415926 * 2.0;
	pitch[frameID] = (int)(floor(log(pitchIncr)/log(2.0)*PITCH_RESOLUTION + PITCH_RESOLUTION * 10.0 + 0.5));
	
	// convert pitch to FFT index
	// This is the player code. I can get from here on...
	//double pitchIncr = pow(2.0, (double)pitch[frameID] * (1.0/24) - 10.0);
	//double freq = pitchIncr * 44100.0 / 3.1415926 / 2.0;
	double fftIndex = freq / 44100.0 * WINDOW_SIZE;

	int overtone = 1;
	for (int band = 0; band < NUM_BANDS; band++)
	{
		int shifter = band/2 - 1;
		if (shifter < 0) shifter = 0;
		int numBandTones = 1 << shifter;
		
		// Get the start - to end index
		int startIndex = (int)((overtone-0.5)*fftIndex);
		int endIndex = (int)((overtone+numBandTones-0.5)*fftIndex);
		
		double energy = 0.0;
		for (int index = startIndex; index < endIndex && index < WINDOW_SIZE/2; index++)
		{
			energy += sqrt(pow(2.0, refSpec[frameID*WINDOW_SIZE+index]));
		}
		// Number of overtones normalization
		energy /= numBandTones;
		if (energy < 1) energy = 1;

		// I am not sure here. The -16 is suspicious
		amplitude[frameID*NUM_BANDS + band] = (int)(log(energy) / log(2.0)) - 9;

		overtone += numBandTones;
	}

	amplitudeDefined[frameID] = true;
	//pitch[frameID] -= 24;
}

// Makes the first (bad) estimate of the synthesis parameters
void initialEstimate()
{
	if (pitch) delete [] pitch;
	pitch = new int [refSpecNumFrames * WINDOW_SIZE];
	if (amplitude) delete [] amplitude;
	amplitude = new int [refSpecNumFrames * NUM_BANDS];
	if (d_amplitude) delete [] d_amplitude;
	d_amplitude = new double [refSpecNumFrames * NUM_BANDS];
	if (amplitudeDefined) delete [] amplitudeDefined;
	amplitudeDefined = new bool [refSpecNumFrames];

	for (int i = 0; i < refSpecNumFrames; i++)
	{
		estimateFrame(i);
	}
}

void init()
{
	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		hannWindow[i] = 0.5 * (1.0 - cos(2.0 * 3.1415926 * i / (WINDOW_SIZE - 1)));
	}

	// Generate noise
	seed = 1;
	// set filter parameters
	double f, p, q;
	//q = 1.0f - 0.005; // 1. - cutoff
	q = 1.0f - 0.01; // 1. - cutoff
	p = 0.01 + 0.8f * 0.01 * q; //cutoffFreq + 0.8f * cutoffFreq * q;
	f = p + p - 1.0f;
	q = 0.0;// / (cutoffFreq + 0.25);
	for (int k = 0; k < 5; k++) filterY[k] = 0.0f;

	for (int samp = 0; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = filter(jfrand()-0.5, f, p, q, 0) * 8.0;
	}
}

void specgram(const double *input, double *output, int numFrames)
{
	for (int frame = 0; frame < numFrames; frame++)
	{
		int inPos = frame * FRAME_STEP;
		int outPos = frame * WINDOW_SIZE;

		// estimate DC offset
		double dcOffset = 0.0;
		for (int sample = 0; sample < WINDOW_SIZE; sample++)
		{
			dcOffset += input[sample+inPos];
		}
		dcOffset /= WINDOW_SIZE;

		for (int sample = 0; sample < WINDOW_SIZE; sample++)
		{
			windowBuffer[sample] = (input[sample+inPos]-dcOffset) * hannWindow[sample];
			output[sample+outPos] = windowBuffer[sample] / 1000.;
		}
#if 1
		fft_object.do_fft(output+outPos, windowBuffer);
		/* Go to Energy domain */
		for (int i = 1; i < WINDOW_SIZE/2; i++)
		{
			output[i+outPos] =
					output[i+outPos] * output[i+outPos] +
					output[i+WINDOW_SIZE/2+outPos] * output[i+WINDOW_SIZE/2+outPos];
		}
		output[outPos] = output[outPos] * output[outPos];
		output[outPos+WINDOW_SIZE/2] = output[outPos+WINDOW_SIZE/2] * output[outPos+WINDOW_SIZE/2];

		// And into log domain
		for (int i = 0; i < WINDOW_SIZE/2; i++)
		{
			if (output[outPos+i] < 256.0*256.0) output[outPos+i] = log(256.0*256.0) / log(2.0);
			else output[outPos+i] = log(output[outPos+i]) / log(2.0);
		}
#endif
	}
}

void loadFile(const char *filename)
{
	int version;

	FILE *fid = fopen(filename, "rb");
	if (fid == NULL)
	{
		return;
	}
	fread(&version, sizeof(version), 1, fid);
	fread(pitch, sizeof(*pitch), refSpecNumFrames, fid);
	fread(amplitude, sizeof(*amplitude), refSpecNumFrames * NUM_BANDS, fid);
	fread(amplitudeDefined, sizeof(*amplitudeDefined), refSpecNumFrames, fid);
	fclose(fid);
}

void exportFile(const char *filename)
{
	FILE *fid = fopen(filename, "w");
	if (fid == NULL)
	{
		return;
	}

	// write the pitch
	fprintf(fid, "char pitch[] = {\n");
	fprintf(fid, "    %d, ", pitch[0]);
	for (int i = 1; i < refSpecNumFrames; i++)
	{
		fprintf(fid, "%d,", pitch[i] - pitch[i-1]);
	}
	fprintf(fid, "\n};\n\n");
	
	// write the skipsize
	fprintf(fid, "char frameStep[] = {\n");

	int step = 0;
	int refSpecNumDefinedFrames = 0;
	for (int i = 0; i < refSpecNumFrames; i++)
	{
		if (amplitudeDefined[i])
		{
			fprintf(fid, "    %d,\n", step);
			step = 0;
			refSpecNumDefinedFrames++;
		}
		else
		{
			step++;
		}
	};
	fprintf(fid, "};\n\n");

	// write the amplitudes (bands in one column)
	fprintf (fid, "char amplitude[][%d] = {\n", NUM_BANDS);
	fprintf (fid, "    {");
	int oldAmplitude = 0;
	for (int i = 0; i < NUM_BANDS; i++)
	{
		fprintf(fid, "%d, ", amplitude[i] - oldAmplitude);
		oldAmplitude = amplitude[i];
	}
	fprintf(fid, "},\n");
	int oldPos = 0;
	for (int j = 1; j < refSpecNumFrames; j++)
	{
		if (amplitudeDefined[j])
		{
			fprintf(fid, "    {");
			int oldOldAmplitude = 0;
			oldAmplitude = 0;
			for (int i = 0; i < NUM_BANDS; i++)
			{
				int pos = j*NUM_BANDS + i;
				int oPos = oldPos*NUM_BANDS + i;
				fprintf(fid, "%d, ", (amplitude[pos] - oldAmplitude) - (amplitude[oPos] - oldOldAmplitude));
				oldAmplitude = amplitude[pos];
				oldOldAmplitude = amplitude[oPos];			
			}
			oldPos = j;
			fprintf(fid, "},\n");
		}
	}
	fprintf(fid, "};\n\n");

	fprintf(fid, "#define NUM_FRAMES %d\n", refSpecNumFrames);
	fprintf(fid, "#define NUM_DEFINED_FRAMES %d\n", refSpecNumDefinedFrames);

	// done.
	fclose(fid);
}

void saveFile(const char *filename)
{
	int version = 1;

	FILE *fid = fopen(filename, "wb");
	if (fid == NULL)
	{
		return;
	}
	fwrite(&version, sizeof(version), 1, fid);
	fwrite(pitch, sizeof(*pitch), refSpecNumFrames, fid);
	fwrite(amplitude, sizeof(*amplitude), refSpecNumFrames * NUM_BANDS, fid);
	fwrite(amplitudeDefined, sizeof(*amplitudeDefined), refSpecNumFrames, fid);
	fclose(fid);
}

void saveWavFile(const char *filename)
{
	FILE *fid = fopen(filename, "wb");
	if (fid == NULL)
	{
		return;
	}
	//if (dirty) undirty();
	fwrite(wavefile, refWaveLength*MZK_NUMCHANNELS*sizeof(short)+44, 1, fid );
	fclose(fid);
}

void loadWave(const char *filename)
{
	/* Load Wave */
	FILE *fid = fopen(filename, "rb");
	if (fid == NULL)
	{
		return;
	}
	fseek(fid, 0, SEEK_END);
	fpos_t len;
	fgetpos(fid, &len);
	fseek(fid, 0, SEEK_SET);
	unsigned int length = (unsigned int)len;
	if (refwavefile) delete [] refwavefile;
	refwavefile = new short [length/2+1];
	if (refWave) delete [] refWave;
	refWave = new double [length/2+1];
	ZeroMemory(refWave, sizeof(double) * (length/2+1));
	fread(refwavefile, 1, length, fid);
	fclose(fid);
	for (unsigned int i = 0; i < length/2 - 32; i++)
	{
		refWave[i] = (double)refwavefile[i + 32];
	}
	refWaveLength = (int)length/2 - 32;
}

unsigned int jetColor(int value)
{
	if (value < 0)
	{
		return 32*4+2;
	}

	if (value < 32)
	{
		return (value + 32) * 4 + 2;
	}
	
	if (value < 96)
	{
		value -= 32;
		return ((value * 4 + 2)<<8) | 255;
	}

	if (value < 160)
	{
		value -= 96;
		return ((value * 4 + 2)<<16) | (255<<8) | ((63-value)*4 + 2);
	}

	if (value < 224)
	{
		value -= 160;
		return (255 << 16) | (((63-value) * 4 + 2) << 8) | 0;
	}

	if (value < 256)
	{
		value -= 224;
		return (((63-value) * 4 + 2) << 16) | (0<<8) | 0;
	}

	return ((32*4) << 16) | (0<<8) | 0;
}

/* Get a filename for load/save */
BOOL selectFile(char *path, bool load)
{
	OPENFILENAME opfn;
	ZeroMemory(&opfn, sizeof(opfn));

	opfn.lStructSize = sizeof(opfn);
	opfn.hwndOwner = hWnd;
	opfn.hInstance = hInst;
	opfn.lpstrFilter = 0;
	opfn.lpstrCustomFilter = 0;
	opfn.nMaxCustFilter = 0;
	opfn.nFilterIndex = 0;
	opfn.lpstrFile = path;
	opfn.nMaxFile = 4090;
	opfn.lpstrFileTitle = 0;
	opfn.nMaxFileTitle = 0;
	opfn.lpstrInitialDir = 0;
	opfn.lpstrTitle = 0;
	opfn.Flags = 0;
	opfn.lpstrDefExt = 0;
	path[0] = 0;

	if (load)
	{
		return GetOpenFileName(&opfn);
	}
	else
	{
		return GetSaveFileName(&opfn);
	}
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Hier Code einfügen.
	MSG msg;
	HACCEL hAccelTable;

	// Globale Zeichenfolgen initialisieren
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_VOICE1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Anwendungsinitialisierung ausführen:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VOICE1));

	// init all the precalc data
	init();

	// Hauptnachrichtenschleife:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNKTION: MyRegisterClass()
//
//  ZWECK: Registriert die Fensterklasse.
//
//  KOMMENTARE:
//
//    Sie müssen die Funktion verwenden,  wenn Sie möchten, dass der Code
//    mit Win32-Systemen kompatibel ist, bevor die RegisterClassEx-Funktion
//    zu Windows 95 hinzugefügt wurde. Der Aufruf der Funktion ist wichtig,
//    damit die kleinen Symbole, die mit der Anwendung verknüpft sind,
//    richtig formatiert werden.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VOICE1));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_VOICE1);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNKTION: InitInstance(HINSTANCE, int)
//
//   ZWECK: Speichert das Instanzenhandle und erstellt das Hauptfenster.
//
//   KOMMENTARE:
//
//        In dieser Funktion wird das Instanzenhandle in einer globalen Variablen gespeichert, und das
//        Hauptprogrammfenster wird erstellt und angezeigt.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Instanzenhandle in der globalen Variablen speichern

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// generate the defined and undefined amplitude frames
void interpolateAmplitude()
{
	for (int frame = 0; frame < refSpecNumFrames; frame++)
	{
		if (amplitudeDefined[frame])
		{
			for (int i = 0; i < NUM_BANDS; i++)
			{
				d_amplitude[frame*NUM_BANDS+i] = (double)amplitude[frame*NUM_BANDS+i];
			}
		}
		else
		{
			int leftFrame = frame - 1;
			while (!amplitudeDefined[leftFrame]) leftFrame--;
			int rightFrame = frame + 1;
			while (!amplitudeDefined[rightFrame]) rightFrame++;
			double t = (frame - leftFrame) / (rightFrame - leftFrame);

			for (int i = 0; i < NUM_BANDS; i++)
			{
				double rightVal = (double)amplitude[rightFrame*NUM_BANDS+i];
				double leftVal = (double)amplitude[leftFrame*NUM_BANDS+i];
				d_amplitude[frame*NUM_BANDS+i] = t * rightVal + (1.0 - t) * leftVal;
			}
		}
	}
}

// Make a dirty sound undirty
void undirty()
{
	interpolateAmplitude();

	// clear wave
	for (int i = 0; i < refWaveLength; i++)
	{
		wave[i] = 0.0;
	}

	// reset the random number generator
	seed = 1;
	int overtone = 1;
	for (int band = 0; band < NUM_BANDS; band++)
	{
		int shifter = band/2 - 1;
		if (shifter < 0) shifter = 0;
		int numBandTones = 1 << shifter;
		// Generate sound
		double curPitch = jfrand() * 3.1415926 * 2.0;
		for (int frame = 0; frame < refSpecNumFrames - 1; frame++)
		{
			double startPitch = (double)pitch[frame];
			double deltaPitch = (double)(pitch[frame+1] - startPitch) * (1.0/FRAME_STEP);
			
			//startPitch -= 24;
			
			double startAmplitude = d_amplitude[frame*NUM_BANDS+band];
			double deltaAmplitude = (d_amplitude[(frame+1)*NUM_BANDS+band] - startAmplitude) * (1.0/FRAME_STEP);

			startAmplitude -= 2.0;

			for (int i = 0; i < FRAME_STEP; i++)
			{
				for (int bandTone = 0; bandTone < numBandTones; bandTone++)
				{	
					//double noisyness = (double)(band) * 0.125f;
					//double noisyness = (double)(band) * (0.0625f + 0.03125f);
					//double noisyness = (double)(overtone+bandTone) * (0.0625f);
					//double noisyness = (double)(overtone+bandTone) * (0.0625f+0.03125f) - 0.5f;
					//double noisyness = (double)(overtone+bandTone) * (0.125f) - 1.0;
					double noisyness = (double)(overtone+bandTone) * (0.03125f) - 0.5;
					if (noisyness > 1.0) noisyness = 1.0;					
					if (noisyness < 0.0) noisyness = 0.0;
					double t = noisyness * 4. * noise[(i+FRAME_STEP*frame+16768*(overtone+bandTone))%NOISE_LENGTH] +
						       (1.0-noisyness);
					double output = sin(curPitch*(overtone+bandTone)) * t;
					wave[i+FRAME_STEP*frame] += pow(2.0, startAmplitude) * output;
				}
				//double jitter = noise[(i+FRAME_STEP*frame)%NOISE_LENGTH] * 0.002;
				//double jitter = sin((i+FRAME_STEP*frame)*0.005) * 0.0004;
				double jitter = 0.0;
				curPitch += pow(2.0, startPitch * (1.0/PITCH_RESOLUTION) - 10.0) + jitter;
				startPitch += deltaPitch;
				startAmplitude += deltaAmplitude;
				while (curPitch > 2.*3.1415926) curPitch -= 2.*3.1415926;
			}
		}

		overtone += numBandTones;
	}

	// calculate spectrogram
	specgram(wave, spec, refSpecNumFrames);

	// copy music to wavefile
	for (int i = 0; i < refWaveLength; i++)
	{
		int data = (int)wave[i];
		if (data > 32767) data = 32767;
		if (data < -32767) data = -32767;
		wavefile[i+sizeof(wavHeader)/2] = (short)(data);
	}

	dirty = false;
}


// Fill the drawing buffer with stuff
void fillBuffer()
{
	ZeroMemory(buffer, sizeof(buffer));	
	if (!refSpec)
	{
		return;
	}

	// recreate the spec if necessary
	if (dirty && renderObject == 1)
	{
		undirty();
	}

	// Do the real rendering
	for (int x = 0; x < min(SXRES, refSpecNumFrames); x++)
	{
		for (int y = 0; y < min(WINDOW_SIZE, SYRES); y++)
		{
			int outPos = (SYRES-1-y)*SXRES + x;
			int inPos = WINDOW_SIZE * x + y;
			switch (renderObject)
			{
			case 0:
				buffer[outPos] = jetColor((int)(refSpec[inPos] * 8.0) - 128);
				break;
			case 1:
				buffer[outPos] = jetColor((int)(spec[inPos] * 8.0) - 128);
				break;
			case 2:
			default:
				//buffer[outPos] = jetColor((int)((refSpec[inPos] - spec[inPos]) * 8.0) + 128);
				buffer[outPos] = jetColor(0);
				break;
			}
		}
	}

	// Render the lines in mode 2
	if (renderObject == 2)
	{
		for (int x = 0; x < min(SXRES, refSpecNumFrames); x++)
		{
			int overtone = 1;
			for (int band = 0; band < NUM_BANDS; band++)
			{
				int shifter = band/2 - 1;
				if (shifter < 0) shifter = 0;
				int numBandTones = 1 << shifter;

				for (int tone = 0; tone < numBandTones; tone++)
				{
					double refPitchIncr = exp(((double)pitch[x] - 240.0) * log(2.0) / PITCH_RESOLUTION);
					int y = (int)(floor((overtone+tone)*refPitchIncr * WINDOW_SIZE / 3.1415926 / 2.0 + 0.5));
					if (y >= 0 && y < SYRES)
					{
						int outPos = (SYRES-1-y)*SXRES + x;
						int inPos = x * NUM_BANDS + band;
						int val = amplitude[inPos];
						if (y > 0) buffer[outPos-SXRES] = jetColor(val*14);
						buffer[outPos] = jetColor(val*16+16);
						if (y < SYRES-1) buffer[outPos+SXRES] = jetColor(val*14);
					}
				}

				overtone += numBandTones;
			}
		}
	}

	// Render the amplitude defined bars
	for (int x = 0; x < min(SXRES, refSpecNumFrames); x++)
	{
		if (!amplitudeDefined[x])
		{
			for (int y = 0; y < SYRES; y++)
			{
				int outPos = y*SXRES + x;
				//buffer[outPos] = ((buffer[outPos] & 0xfcfcfcfc) >> 2) + 0x20202020;
				double color = (buffer[outPos] & 0xff) * 0.25 +
					           ((buffer[outPos] >> 8) & 0xff) * 0.4 +
							   ((buffer[outPos] >> 16) & 0xff) * 0.35;
				int col = (int)(color * 2 / 3);
				buffer[outPos] = ((buffer[outPos] & 0xfcfcfcfc) >> 2) + 
								 (col | (col << 8) | (col << 16));
			}
		}
	}
}

//
//  FUNKTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ZWECK:  Verarbeitet Meldungen vom Hauptfenster.
//
//  WM_COMMAND	- Verarbeiten des Anwendungsmenüs
//  WM_PAINT	- Zeichnen des Hauptfensters
//  WM_DESTROY	- Beenden-Meldung anzeigen und zurückgeben
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT clientRect;
	char filename[4096];
	int xPos, yPos, frame;
	double freq;
	int band;

	xPos = LOWORD(lParam); 
	yPos = HIWORD(lParam);
	GetClientRect(hWnd, &clientRect);
	xPos -= clientRect.left;
	yPos -= clientRect.top;
	yPos = (clientRect.bottom - clientRect.top) - yPos - 1;
	freq = yPos * (float)SYRES / (float)(clientRect.bottom - clientRect.top);
	frame = 0;
	band = 0;
	if (clientRect.right - clientRect.left > 0 && refSpecNumFrames > 0)
	{
		frame = min(SXRES, refSpecNumFrames) * xPos / (clientRect.right - clientRect.left);
		if (frame < 0) frame = 0;
		if (frame >= refSpecNumFrames) frame = refSpecNumFrames - 1;

		// get the overtone from the freq.. ?
		double hzfreq = (freq) * 44100 / WINDOW_SIZE;
		double refPitchIncr = exp(((double)pitch[frame] - 240.0) * log(2.0) / PITCH_RESOLUTION);
		double refFreq = refPitchIncr * 44100.0 / 3.1415926 / 2.0;
		//double pitchIncr = hzfreq / 44100.0 * 3.1415926 * 2.0;
		//double pitchcmp = log(pitchIncr)/log(2.0)*24.0 + 24.0 * 10.0;
		int overtone = floor(hzfreq / refFreq - 0.5);
		int cmpOvertone = 0;

		for (int b = 0; b < NUM_BANDS; b++)
		{
			int shifter = b/2 - 1;
			if (shifter < 0) shifter = 0;
			int numBandTones = 1 << shifter;
			band = b;
			if (cmpOvertone+numBandTones > overtone)
			{			
				break;
			}
			cmpOvertone += numBandTones;
		}
	}

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Menüauswahl bearbeiten:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_LOAD_WAV:
			if (selectFile(filename, true))
			{
				// A file was selected, load data
				loadWave(filename);
				if (wave) delete [] wave;
				wave = new double[refWaveLength];
				ZeroMemory(wave, sizeof(double) * refWaveLength);
				refSpecNumFrames = refWaveLength / FRAME_STEP - WINDOW_SIZE/FRAME_STEP + 1;
				if (refSpec) delete [] refSpec;
				refSpec = new double [refSpecNumFrames * WINDOW_SIZE];
				if (spec) delete [] spec;
				spec = new double [refSpecNumFrames * WINDOW_SIZE];
				if (wavefile) delete [] wavefile;
				wavefile = new short [refWaveLength + sizeof(wavHeader)];
				wavHeader[1] = refWaveLength*MZK_NUMCHANNELS*sizeof(short)+36;
				wavHeader[10] = refWaveLength*MZK_NUMCHANNELS*sizeof(short);
				memcpy(wavefile, wavHeader, sizeof(wavHeader));
				specgram(refWave, refSpec, refSpecNumFrames);
				initialEstimate();
				dirty = true;
				InvalidateRect(hWnd, 0, false);
			}
			break;
		case IDM_SAVE_WAV:
			if (selectFile(filename, false))
			{
				// A file was selected, load data
				saveWavFile(filename);
			}
			break;
		case IDM_EXPORT:
			if (selectFile(filename, false))
			{
				exportFile(filename);
			}
			break;
		case IDM_SAVE_FILE:
			if (selectFile(filename, false))
			{
				// A file was selected, load data
				saveFile(filename);
			}
			break;
		case IDM_LOAD_FILE:
			if (selectFile(filename, true))
			{
				// A file was selected, load data
				loadFile(filename);
				dirty = true;
				InvalidateRect(hWnd, 0, false);
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	
	case WM_PAINT:
		// Recreate the buffer for drawing
		fillBuffer();

		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd,&clientRect);
		if (refSpecNumFrames > 0)
			StretchDIBits(hdc,clientRect.left,clientRect.top,clientRect.right,clientRect.bottom,0,0,min(SXRES, refSpecNumFrames),SYRES,buffer,&bmi,DIB_RGB_COLORS,SRCCOPY);
		else
			StretchDIBits(hdc,clientRect.left,clientRect.top,clientRect.right,clientRect.bottom,0,0,SXRES,SYRES,buffer,&bmi,DIB_RGB_COLORS,SRCCOPY);
		// TODO: Hier den Zeichnungscode hinzufügen.
		EndPaint(hWnd, &ps);
		break;
	
	case WM_LBUTTONDOWN:
		if (wParam & MK_CONTROL)
		{
			amplitude[frame*NUM_BANDS + band] = 0;
		}
		else
		{
			if (wParam & MK_SHIFT)
			{
				amplitude[frame*NUM_BANDS + band]++;
			}
			else
			{
				if (frame > 0 && frame < refSpecNumFrames-1)
				{
					amplitudeDefined[frame] = !amplitudeDefined[frame];
				}
			}
		}
		dirty = true;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_RBUTTONDOWN:
		if (wParam & MK_SHIFT)
		{
			if (amplitude[frame*NUM_BANDS + band] > 0)
			{
				amplitude[frame*NUM_BANDS + band]--;
			}
		}
		else
		{
			if (frame >= 0 && frame <= refSpecNumFrames-1)
			{
				bool oldAct = amplitudeDefined[frame];
				estimateFrame(frame, freq);
				amplitudeDefined[frame] = oldAct;
			}
		}
		dirty = true;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'a':
		case 'A':
			renderObject = (renderObject + 1) % 3;
			InvalidateRect(hWnd, 0, false);
			break;

		case 'o':
		case 'O':
			sndPlaySound( (const char*)refwavefile, SND_ASYNC|SND_MEMORY );
			break;

		case 'p':
		case 'P':
			if (dirty) undirty();
			sndPlaySound( (const char*)wavefile, SND_ASYNC|SND_MEMORY );
			break;

		default:
			break;
		}
		break;
	
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Meldungshandler für Infofeld.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
