#include "StdAfx.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "glext.h"
#include "gl/glu.h"
#include "stb_image.h"

float noiseData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
unsigned char noiseIntData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
float noiseData3D[TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * 4];

static int open_codec_context(int *stream_idx,
    AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        //fprintf(stderr, "Could not find %s stream in input file '%s'\n",
        //    av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            //fprintf(stderr, "Failed to find %s codec\n",
            //    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", "0", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            //fprintf(stderr, "Failed to open %s codec\n",
            //    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

TextureManager::TextureManager(void)
{
	numTextures = 0;
	numVideos = 0;
}

TextureManager::~TextureManager(void)
{
}

void TextureManager::releaseAll(void)
{
	glDeleteTextures(numTextures, textureID);
	numTextures = 0;

	for (int i = 0; i < numVideos; i++)
	{
        // Free the RGB image
        av_free(video_buffer_[i]);
        av_free(video_frame_rgb_[i]);
        // Free the YUV frame
        av_free(video_frame_[i]);
        // Close the codecs
        avcodec_close(video_codec_context_[i]);
        avcodec_close(video_codec_context_orig_[i]);
        // Close the video file
        avformat_close_input(&video_format_context_[numVideos]);
	}
	glDeleteTextures(numVideos, videoTextureID);
	numVideos = 0;
}

int TextureManager::loadAVI(const char *filename, char *errorString)
{
	char combinedName[TM_MAX_FILENAME_LENGTH+1];
	sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
			  TM_DIRECTORY "%s", filename);

	if (numTextures >= TM_MAX_NUM_TEXTURES) {
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

    // Open video file
    if (avformat_open_input(&video_format_context_[numVideos], combinedName, NULL, NULL) != 0) {
        sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not open video file '%s'", filename);
        return -1; // Couldn't open file
    }
    // Retrieve stream information
    if (avformat_find_stream_info(video_format_context_[numVideos], NULL) < 0) {
        sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not retrieve stream information from %s",
                filename);
        return -1; // Couldn't find stream information
    }

    if (open_codec_context(&video_stream_[numVideos], video_format_context_[numVideos],
            AVMEDIA_TYPE_VIDEO) < 0) {
        sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not find video codec in %s", filename);
        return -1;
    }

    video_codec_context_[numVideos] = video_format_context_[numVideos]->streams[video_stream_[numVideos]]->codec;

    /* allocate image where the decoded image will be put */
    videoWidth[numVideos] = video_codec_context_[numVideos]->width;
    videoHeight[numVideos] = video_codec_context_[numVideos]->height;
    
    // Allocate video frame
    video_frame_[numVideos] = av_frame_alloc();
    // Allocate an AVFrame structure
    video_frame_rgb_[numVideos] = av_frame_alloc();    
    int num_bytes;
    // Determine required buffer size and allocate buffer
    videoWidth[numVideos] = video_codec_context_[numVideos]->width;
    videoHeight[numVideos] = video_codec_context_[numVideos]->height; 
    num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
        videoWidth[numVideos], videoHeight[numVideos], 1);
    video_buffer_[numVideos] = (unsigned char *)av_malloc(num_bytes * sizeof(unsigned char));
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
    av_image_fill_arrays(video_frame_rgb_[numVideos]->data, video_frame_rgb_[numVideos]->linesize,
                         video_buffer_[numVideos], AV_PIX_FMT_RGB24,
                         videoWidth[numVideos], videoHeight[numVideos], 1);

    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&(video_packet_[numVideos]));
    video_packet_[numVideos].data = NULL;
    video_packet_[numVideos].size = 0;

    // Signal that we are at the beginning of the stream
    video_next_frame_index_[numVideos] = 0;

    // initialize SWS context for software scaling
    sws_ctx_[numVideos] = sws_getContext(video_codec_context_[numVideos]->width,
        video_codec_context_[numVideos]->height,
        video_codec_context_[numVideos]->pix_fmt,
        video_codec_context_[numVideos]->width,
        video_codec_context_[numVideos]->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);

	// create openGL texture
	int textureSize = videoWidth[numVideos] * videoHeight[numVideos] * 3;
	glGenTextures(1, &videoTextureID[numVideos]);
	glBindTexture(GL_TEXTURE_2D, videoTextureID[numVideos]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 videoWidth[numVideos], videoHeight[numVideos],
				 0, GL_BGR, GL_UNSIGNED_BYTE, video_buffer_[numVideos]);
	strcpy_s(videoName[numVideos], TM_MAX_FILENAME_LENGTH, filename);

	numVideos++;
	return 0;
}

int TextureManager::loadTGA(const char *filename, char *errorString)
{
	char combinedName[TM_MAX_FILENAME_LENGTH+1];

	TGAHeader tgaHeader;
	int numRead;

	sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
			  TM_DIRECTORY "%s", filename);

	if (numTextures >= TM_MAX_NUM_TEXTURES)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

	FILE *fid;
	if (fopen_s(&fid, combinedName, "rb"))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not load texture '%s'",
				  filename);
		return -1;
	}

	// load header
	numRead = fread(&tgaHeader, 1, sizeof(tgaHeader), fid);
	if (numRead != sizeof(tgaHeader))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "TGA Header of %s too short",
			      filename);
		return -1;
	}
		
	// load image data
	int textureSize = tgaHeader.width * tgaHeader.height * 4;
	unsigned char *textureData = new unsigned char[textureSize];
	numRead = fread(textureData, 1, textureSize, fid);
	if (numRead != textureSize)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Bogus tga file: '%s'",
			      filename);
		delete [] textureData;
		return -1;
	}

    // Pre-multiply alpha
    for (int pos = 0; pos < tgaHeader.width * tgaHeader.height; pos++) {
        int index = pos * 4;
        int alpha = 3;
        for (int color = 0; color < 3; color++) {
            textureData[index + color] = (((int)textureData[index + color]) *
                ((int)textureData[index + alpha])) / 255;
        }
    }
		
	// create openGL texture
	glGenTextures(1, &textureID[numTextures]);
	textureWidth[numTextures] = tgaHeader.width;
	textureHeight[numTextures] = tgaHeader.height;
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
#if 0 // Use simple mipmap generation	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 textureWidth[numTextures], textureHeight[numTextures],
				 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#else
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
					  textureWidth[numTextures], textureHeight[numTextures],
					  GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#endif

	delete [] textureData;
	fclose(fid);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, filename);
	numTextures++;

	return 0;
}

int TextureManager::loadPNG(const char *filename, char *errorString)
{
    char combinedName[TM_MAX_FILENAME_LENGTH+1];

    sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
        TM_DIRECTORY "%s", filename);

    if (numTextures >= TM_MAX_NUM_TEXTURES) {
        sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
        return -1;
    }

    int width, height, num_components;
    unsigned char *data = stbi_load(combinedName, &width, &height, &num_components, 4);

    // Pre-multiply alpha
    for (int pos = 0; pos < width * height; pos++) {
        int index = pos * 4;
        int alpha = 3;
        for (int color = 0; color < 3; color++) {
            data[index + color] = (((int)data[index + color]) *
                ((int)data[index + alpha])) / 255;
        }
    }

    // create openGL texture
    glGenTextures(1, &textureID[numTextures]);
    textureWidth[numTextures] = width;
    textureHeight[numTextures] = height;
    glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
#if 0 // Use simple mipmap generation	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        textureWidth[numTextures], textureHeight[numTextures],
        0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#else
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
        textureWidth[numTextures], textureHeight[numTextures],
        GL_RGBA, GL_UNSIGNED_BYTE, data);
#endif
    
    stbi_image_free(data);

    strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, filename);
    numTextures++;

    return 0;
}

int TextureManager::init(char *errorString, HDC mainDC)
{
	// Free everything if there was something before.
	releaseAll();

	// Initialize video rendering
    av_register_all();

	// create small and large rendertarget texture
	createRenderTargetTexture(errorString, X_OFFSCREEN, Y_OFFSCREEN, TM_OFFSCREEN_NAME);
	createRenderTargetTexture(errorString, X_HIGHLIGHT, Y_HIGHLIGHT, TM_HIGHLIGHT_NAME);
	createNoiseTexture(errorString, TM_NOISE_NAME);

	// Go throught the shaders directory and load all shaders.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in textures directory
	char *dirname = TM_DIRECTORY TM_SHADER_WILDCARD;
	hFind = FindFirstFile(TM_DIRECTORY TM_SHADER_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no textures in " TM_DIRECTORY);
		return -1;
	}

	// Load all the textures in the directory
	do
	{		
		// Note that the number of textures is increased automatically
		int retVal = loadPNG(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	// Go to video first file in textures directory
	hFind = INVALID_HANDLE_VALUE;
	dirname = TM_DIRECTORY TM_VIDEO_WILDCARD;
	hFind = FindFirstFile(TM_DIRECTORY TM_VIDEO_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no videos in " TM_DIRECTORY);
		// No videos are not necessarily an error.
		return 0;
	}

	// Load all the videos in the directory
	do
	{		
		// Note that the number of videos is increased automatically
		int retVal = loadAVI(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	return 0;
}

int TextureManager::getVideoID(const char *name, GLuint *id, char *errorString, float frame_time)
{	
	for (int i = 0; i < numVideos; i++)
	{
		if (strcmp(name, videoName[i]) == 0)
		{
			*id = videoTextureID[i];

            AVRational time_base = video_format_context_[i]->streams[video_stream_[i]]->time_base;
            //int64_t current_frame_pts = video_frame_[i]->best_effort_timestamp;
            int64_t current_frame_pts = av_frame_get_best_effort_timestamp(video_frame_[i]);
            float current_frame_time = (float)(time_base.num) * (float)current_frame_pts /
                (float)(time_base.den);
            int64_t desired_time_stamp = (int64_t)(frame_time * (float)time_base.den / (float)time_base.num);

            // Decode and re-format single frame
            if (!sws_ctx_[i] ||
                desired_time_stamp > video_format_context_[i]->streams[video_stream_[i]]->duration) {
                // Could not get frame from video, use black instead?
                int retVal = getTextureID("black.png", id, errorString);
                if (retVal < 0) return retVal;
                else return 1; // mark finished
                // TODO (jhofer): Or should I just use break to use the last frame.
            } 

            const float kMaxFrameTimeError = 1.0f / 8.0f;
            const float kPreTime = 1.0f / 20.0f;
            if (current_frame_pts >= 0 &&
                current_frame_time > frame_time + kMaxFrameTimeError) {
                // the displayed frame is ahead of what we want to show
                av_seek_frame(video_format_context_[i], video_stream_[i],
                    desired_time_stamp - (int64_t)((kPreTime * (float)time_base.den) / (float)time_base.num), AVSEEK_FLAG_BACKWARD);
                current_frame_pts = -1;  // Mark that we are way off
            }
            if (current_frame_pts >= 0 &&
                current_frame_time < frame_time - kMaxFrameTimeError) {
                // the displayed frame is way behind of what we want to show
                av_seek_frame(video_format_context_[i], video_stream_[i],
                    desired_time_stamp - (int64_t)((kPreTime * (float)time_base.den) / (float)time_base.num), AVSEEK_FLAG_BACKWARD);
                current_frame_pts = -1;  // Mark that we are way off
            }

            while (current_frame_pts < 0 || current_frame_time < frame_time) {
                // We have to decode up to 1/10th of a second, or are at a no-idea frame
                int frameFinished = 0;
                while (!frameFinished) {
                    if (video_packet_[i].size <= 0) {
                        if (av_read_frame(video_format_context_[i], &(video_packet_[i])) < 0) {
                            // Could not get frame from video, use black instead?
                            int retVal = getTextureID("black.png", id, errorString);
                            if (retVal < 0) return retVal;
                            else return 1; // mark finished
                            // TODO (jhofer): Or should I just use break to use the last frame.
                        }
                        video_packet_orig_[i] = video_packet_[i];
                    }
                    if (video_packet_[i].stream_index == video_stream_[i]) {
                        // Decode video frame
                        int ret = avcodec_decode_video2(video_codec_context_[i], video_frame_[i],
                            &frameFinished, &video_packet_[i]);
                        if (ret < 0) {
                            sprintf_s(errorString, MAX_ERROR_LENGTH, "Error decoding stream in '%s'", name);
                            return -1;
                        }

                        // Did we get a video frame?
                        if(frameFinished) {
                            // Convert the image from its native format to RGB
                            sws_scale(sws_ctx_[i], (uint8_t const * const *)video_frame_[i]->data,
                                video_frame_[i]->linesize, 0, video_codec_context_[i]->height,
                                video_frame_rgb_[i]->data, video_frame_rgb_[i]->linesize);
                        }

                        video_packet_[i].data += ret;
                        video_packet_[i].size -= ret;
                        if (video_packet_[i].size <= 0) {
                            av_packet_unref(&video_packet_orig_[i]);
                            video_packet_[i].size = 0;
                        }
                    } else {
                        // Un-reference unused audio packet
                        av_packet_unref(&video_packet_orig_[i]);
                        video_packet_[i].size = 0;  // mark as empty
                    }
                }

                current_frame_pts = av_frame_get_best_effort_timestamp(video_frame_[i]);
                current_frame_time = (float)(time_base.num) * (float)current_frame_pts /
                    (float)(time_base.den);
                //av_frame_unref(video_frame_[i]);
            }

            // write openGL texture
			glEnable(GL_TEXTURE_2D);				// Enable Texture Mapping
			glBindTexture(GL_TEXTURE_2D, *id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoWidth[i], videoHeight[i],
                GL_RGB, GL_UNSIGNED_BYTE, video_frame_rgb_[i]->data[0]);

			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find video '%s'", name);
	return -1;
}

int TextureManager::getTextureID(const char *name, GLuint *id, char *errorString)
{
	for (int i = 0; i < numTextures; i++)
	{
		if (strcmp(name, textureName[i]) == 0)
		{
			*id = textureID[i];
			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find texture '%s'", name);
	return -1;
}

int TextureManager::createNoiseTexture(char *errorString, const char *name)
{
	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	generateNoiseTexture();
	makeIntTexture();
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
					  TM_NOISE_TEXTURE_SIZE, TM_NOISE_TEXTURE_SIZE,
					  GL_BGRA, GL_UNSIGNED_BYTE, noiseIntData);
	textureWidth[numTextures] = TM_NOISE_TEXTURE_SIZE;
	textureHeight[numTextures] = TM_NOISE_TEXTURE_SIZE;
	numTextures++;
	return 0;
}

int TextureManager::createRenderTargetTexture(char *errorString, int width, int height,
										      const char *name)
{
	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
		         GL_UNSIGNED_BYTE, 0);
	textureWidth[numTextures] = width;
	textureHeight[numTextures] = height;
	numTextures++;
	return 0;
}

float TextureManager::frand(void)
{
	return (float)(rand()) / (float)RAND_MAX;
}

// normalize filter kernel
void TextureManager::normalizeKernel(float *kernel, int kernelLength)
{
	float sum = 0.0f;

	for (int i = 0; i < kernelLength; i++)
	{
		sum += kernel[i];
	}

	for (int i = 0; i < kernelLength; i++)
	{
		kernel[i] /= sum;
	}
}

// set min/max to [0..1]
void TextureManager::normalizeTexture(float *texture, int textureSize)
{
	float min[4] = {1.0e20f, 1.0e20f, 1.0e20f, 1.0e20f};
	float max[4] = {-1.0e20f, -1.0e20f, -1.0e20f, -1.0e20f};

	// estimate
	for (int i = 0; i < textureSize; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			if (texture[i*4+k] < min[k]) min[k] = texture[i*4+k];
			if (texture[i*4+k] > max[k]) max[k] = texture[i*4+k];
		}
	}

	// apply
	for (int i = 0; i < textureSize; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			texture[i*4+k] -= min[k];
			texture[i*4+k] /= (max[k] - min[k]);
		}
	}
}

void TextureManager::makeIntTexture(void)
{
	for (int i = 0; i < TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4; i++)
	{
		if (noiseData[i] < 0.0f)
		{
			noiseIntData[i] = 0;
		}
		else if (noiseData[i] >= 1.0f)
		{
			noiseIntData[i] = 255;
		}
		else
		{
			noiseIntData[i] = (unsigned char)(noiseData[i] * 256.0f);
		}
	}
}

// function that generates the 2D noise texture
void TextureManager::generateNoiseTexture(void)
{
	float *largeNoiseData1, *largeNoiseData2;
	float *filterKernel;
	// With copied left-right
	largeNoiseData1 = new float [TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	largeNoiseData2 = new float [TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	filterKernel = new float [TM_FILTER_KERNEL_SIZE];

	// Generate white noise baseline
	for (int i = 0; i < TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand();
	}

	// copy noise baseline in repeated border buffer
	for (int y = 0; y < TM_NOISE_TEXTURE_SIZE * 2; y++)
	{
		for (int x = 0; x < TM_NOISE_TEXTURE_SIZE * 2; x++)
		{
			int srcIdx = (y % TM_NOISE_TEXTURE_SIZE) * TM_NOISE_TEXTURE_SIZE * 4
					   + (x % TM_NOISE_TEXTURE_SIZE) * 4;
			int dstIdx = y * TM_NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
		}
	}

	// genereate filter kernel
	for (int i = 0; i < TM_FILTER_KERNEL_SIZE; i++)
	{
		float position = (float)i / (float)TM_FILTER_KERNEL_SIZE * 2.0f * 3.1415926f;
		filterKernel[i] = 1.0f - cosf(position);
		//filterKernel[i] = 1.0f;
	}
	normalizeKernel(filterKernel, TM_FILTER_KERNEL_SIZE);

	float *curSrc = largeNoiseData1;
	float *curDst = largeNoiseData2;
	for (int mode = 0; mode < 2; mode++)
	{
		for (int y = 0; y < TM_NOISE_TEXTURE_SIZE + TM_FILTER_KERNEL_SIZE; y++)
		{
			for (int x = 0; x < TM_NOISE_TEXTURE_SIZE; x++)
			{
				float filtered[4];
				filtered[0] = 0.0f;
				filtered[1] = 0.0f;
				filtered[2] = 0.0f;
				filtered[3] = 0.0f;

				int srcIdx = y * TM_NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;
				for (int xp = 0; xp < TM_FILTER_KERNEL_SIZE; xp++)
				{
					filtered[0] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[1] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[2] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[3] += curSrc[srcIdx++] * filterKernel[xp];
				}

				int dstIdx = x * TM_NOISE_TEXTURE_SIZE * 2 * 4 + y * 4;
				curDst[dstIdx] = filtered[0];
				curDst[dstIdx+1] = filtered[1];
				curDst[dstIdx+2] = filtered[2];
				curDst[dstIdx+3] = filtered[3];
			}
		}

		curSrc = largeNoiseData2;
		curDst = largeNoiseData1;
	}

	// copy back to small array
	for (int y = 0; y < TM_NOISE_TEXTURE_SIZE; y++)
	{
		for (int x = 0; x < TM_NOISE_TEXTURE_SIZE; x++)
		{
			int dstIdx = y * TM_NOISE_TEXTURE_SIZE * 4 + x * 4;
			int srcIdx = y * TM_NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;

			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
		}
	}

	// normalize small Array
	normalizeTexture(noiseData, TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE);

	delete [] largeNoiseData1;
	delete [] largeNoiseData2;
	delete [] filterKernel;

	makeIntTexture();
}
