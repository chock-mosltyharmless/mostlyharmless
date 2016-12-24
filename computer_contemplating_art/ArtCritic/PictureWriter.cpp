#include "stdafx.h"
#include "PictureWriter.h"

PictureWriter::PictureWriter(void)
{
}

PictureWriter::~PictureWriter(void)
{
}

void PictureWriter::SaveTGA(int width, int height, float (*texture_data)[3], const char * filename) {
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
    tgaHeader.width = width;
    tgaHeader.height = height;
    tgaHeader.bits = 24;
    tgaHeader.descriptor = 0;

    unsigned char * write_data = new unsigned char[width * height * 3];

    int write_pos = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < 3; c++) {
                write_data[write_pos++] =
                    (int)(texture_data[(height - y - 1) * width + x][2 - c] * 255);
            }
        }
    }

    FILE *fid = NULL;
    fopen_s(&fid, filename, "wb");
    if (fid) {
        fwrite(&tgaHeader, sizeof(tgaHeader), 1, fid);
        fwrite(write_data, 1, width * height * 3, fid);
        fclose(fid);
    }
}

