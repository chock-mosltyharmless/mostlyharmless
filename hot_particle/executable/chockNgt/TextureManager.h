/**
 * The texture manager holds all the textures that the system supports.
 * Additionally to the textures in the ./textures directory, it also
 * holds some special textures. Here is a list of textures:
 * TODO: Shall the texture manager handle the render target stuff or not????
 * - 'noise2D': Core 2D noise texture. In the future there might be more
 * - 'noise3D': Core 3D noise texture.
 * - 'renderTarget': Core render target at render target resolution
 * - 'smallTarget': Smaller render target for hypnoglow and the like
 */

#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libswscale/swscale.h"
}

#define TM_DIRECTORY "textures/"
#define TM_SHADER_WILDCARD "*.tga"
#define TM_VIDEO_WILDCARD "*.wmv"
#define TM_MAX_NUM_TEXTURES 128
#define TM_MAX_NUM_VIDEOS 16
#define TM_MAX_FILENAME_LENGTH 1024

#define TM_OFFSCREEN_NAME "renderTarget"
#define TM_HIGHLIGHT_NAME "smallTarget"
#define TM_NOISE_NAME "noise2D"
#define TM_NOISE3D_NAME "noise3D"

// The noise texture dimension
#define TM_NOISE_TEXTURE_SIZE 256
#define TM_FILTER_KERNEL_SIZE 32

// Name of the 32x32x32 noise texture
#define TM_3DNOISE_TEXTURE_SIZE 16 // try smaller?

struct TGAHeader
{
	unsigned char identSize;
	unsigned char colourmapType;
	unsigned char imageType;

	// This is a stupid hack to fool the compiler.
	// I do not know what happens if I compile it
	// under release conditions.
	unsigned char colourmapStart1;
	unsigned char colourmapStart2;
	unsigned char colourmapLength1;
	unsigned char colourmapLength2;
	unsigned char colourmapBits;

	short xStart;
	short yStart;
	short width;
	short height;
	unsigned char bits;
	unsigned char descriptor;
};

class TextureManager
{
public:
	TextureManager(void);
	~TextureManager(void);

public: // functions
	// Load all textures from the ./textures directory.
	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least SM_MAX_ERROR_LENGTH
	// characters. It contains errors from compilation/loading
	int init(char *errorString, HDC mainDC);

	// Get the texture ID from a named texture
	// That might either be a .tga or any of the special textures
	int getTextureID(const char *name, GLuint *id, char *errorString);

	// Get the texture ID from a named video (this also sets the video texture in openGL)
    // Time is the current time in the video, you get a frame after that time.
    // Time must rise (no back-paddeling)
	int getVideoID(const char *name, GLuint *id, char *errorString, float frame_time);

private: // functions
	void releaseAll(void);
	int createRenderTargetTexture(char *errorString, int width, int height,
								  const char *name);
	int loadTGA(const char *filename, char *errorString);
	int loadAVI(const char *filename, char *errorString);
	float frand(void);
	void normalizeKernel(float *kernel, int kernelLength);
	void normalizeTexture(float *texture, int textureSize);
	void makeIntTexture(void);
	void generateNoiseTexture(void);
	int createNoiseTexture(char *errorString, const char *name);

private: // data
	int numTextures;
	char textureName[TM_MAX_NUM_TEXTURES][TM_MAX_FILENAME_LENGTH+1];
	GLuint textureID[TM_MAX_NUM_TEXTURES];
	int textureWidth[TM_MAX_NUM_TEXTURES];
	int textureHeight[TM_MAX_NUM_TEXTURES];

	// The same for the videos (including textures)
	int numVideos;
	char videoName[TM_MAX_NUM_VIDEOS][TM_MAX_FILENAME_LENGTH+1];
	GLuint videoTextureID[TM_MAX_NUM_VIDEOS];
	int videoWidth[TM_MAX_NUM_VIDEOS];
	int videoHeight[TM_MAX_NUM_VIDEOS];
#if 0
	AVISTREAMINFO psi[TM_MAX_NUM_VIDEOS];
	PAVISTREAM pavi[TM_MAX_NUM_VIDEOS];
	PGETFRAME pgf[TM_MAX_NUM_VIDEOS];
	HBITMAP hBitmap[TM_MAX_NUM_VIDEOS]; // Bitmap that holds the video texture
	unsigned char *videoData[TM_MAX_NUM_VIDEOS];
	HDRAWDIB hdd; // Used for scaling/drawing the avi to a RAM buffer
	HDC hdc;
#else
    AVFormatContext *video_format_context_[TM_MAX_NUM_VIDEOS];
    AVCodecContext *video_codec_context_[TM_MAX_NUM_VIDEOS];
    AVCodecContext *video_codec_context_orig_[TM_MAX_NUM_VIDEOS];
    AVCodec *video_codec_[TM_MAX_NUM_VIDEOS];
    AVFrame *video_frame_[TM_MAX_NUM_VIDEOS];
    AVFrame *video_frame_rgb_[TM_MAX_NUM_VIDEOS];
    int video_stream_[TM_MAX_NUM_VIDEOS];
    unsigned char *video_buffer_[TM_MAX_NUM_VIDEOS];
    AVPacket video_packet_[TM_MAX_NUM_VIDEOS];
    AVPacket video_packet_orig_[TM_MAX_NUM_VIDEOS];
    struct SwsContext *sws_ctx_[TM_MAX_NUM_VIDEOS];
    int video_next_frame_index_[TM_MAX_NUM_VIDEOS];
#endif
};

