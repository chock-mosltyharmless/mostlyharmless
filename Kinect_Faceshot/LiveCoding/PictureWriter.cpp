#include "StdAfx.h"
#include "PictureWriter.h"

PictureWriter::PictureWriter(void)
{
	data_ = NULL;
}

PictureWriter::~PictureWriter(void)
{
	if (data_) delete[] data_;
	data_ = NULL;
}

void PictureWriter::Init(int width, int height) {
	width_ = width;
	height_ = height;
	data_ = new unsigned char[width*height][3];

	// Black it out (transparent)
	FillColor(0, 0, 0, 0);
}

void PictureWriter::FillColor(unsigned char red, unsigned char green,
							  unsigned char blue, unsigned char alpha) {
	for (int i = 0; i < width_*height_; i++) {
		data_[i][0] = blue;
		data_[i][1] = green;
		data_[i][2] = red;
        //data_[i][3] = alpha;
	}
}

void PictureWriter::FillCircle(float x, float y, float radius,
                               unsigned char red, unsigned char green,
                               unsigned char blue, unsigned char alpha) {
    x *= width_;
    y *= height_;
    radius *= width_;

    int left = (int)(x - radius - 0.5f);
    int right = (int)(x + radius + 1.499f);
    int top = (int)(y - radius - 0.5f);
    int bottom = (int)(y + radius + 1.499f);
    if (left < 0) left = 0;
    if (right > width_ - 1) right = width_ - 1;
    if (top < 0) top = 0;
    if (bottom > height_ - 1) bottom = height_ - 1;

    for (int yp = top; yp <= bottom; yp++) {
        int index = yp * width_ + left;
        
        for (int xp = left; xp <= right; xp++) {
            float dist = sqrtf((xp - x) * (xp - x) + (yp - y) * (yp - y));
            if (dist < radius + 0.5f) {
                int real_alpha = alpha;
                int real_red = red;
                int real_green = green;
                int real_blue = blue;
                if (dist > radius - 0.5f) {
                    float alpha_dist = radius + 0.5f - dist;
                    real_alpha = (int)(alpha * alpha_dist + 0.5f);
                    real_red = (int)(red * alpha_dist + 0.5f);
                    real_green = (int)(green * alpha_dist + 0.5f);
                    real_blue = (int)(blue * alpha_dist + 0.5f);
                }
                int new_blue = data_[index][0] * (255 - real_alpha) / 255 + real_blue;
                if (new_blue > 255) new_blue = 255;
                int new_green = data_[index][1] * (255 - real_alpha) / 255 + real_green;
                if (new_green > 255) new_green = 255;
                int new_red = data_[index][2] * (255 - real_alpha) / 255 + real_red;
                if (new_red > 255) new_red = 255;
                //int alpha_background = data_[index][3] * (255 - real_alpha) / 255;
                data_[index][0] = new_blue;
                data_[index][1] = new_green;
                data_[index][2] = new_red;
                //data_[index][3] = alpha_background + real_alpha;
            }
            index++;
        }
    }
}


void PictureWriter::Save(const char *filename) const {
	FILE *fid;

	// generate TGA header
	TGAHeader tgaHeader;
	tgaHeader.identSize = 0;
	tgaHeader.colourmapType = 0;
	tgaHeader.imageType = 2;
	tgaHeader.colourmapStart1 = 0; tgaHeader.colourmapStart2 = 0;
	tgaHeader.colourmapLength1 = 0; tgaHeader.colourmapLength2 = 0;
	tgaHeader.colourmapBits = 0;
	tgaHeader.xStart = 0;
	tgaHeader.yStart = 0;
	tgaHeader.width = width_;
	tgaHeader.height = height_;
	tgaHeader.bits = 24;
	tgaHeader.descriptor = 0;	

	// write tga [optional]
	fopen_s(&fid, filename, "wb");
	if (fid) {
		fwrite(&tgaHeader, sizeof(tgaHeader), 1, fid);
		fwrite(data_, 1, width_ * height_ * 3, fid);
		fclose(fid);
	}
}