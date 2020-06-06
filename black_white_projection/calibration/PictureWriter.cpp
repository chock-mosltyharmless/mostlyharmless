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

    unsigned char *write_data = new unsigned char[width * height * 3];

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

    delete [] write_data;
}

void PictureWriter::SaveTGA(int width, int height, int(*texture_data)[2], const char * filename, int max_value)
{
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
    tgaHeader.bits = 32;
    tgaHeader.descriptor = 0;

    unsigned char *write_data = new unsigned char[width * height * 4];

    int write_pos = 0;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            write_data[write_pos++] = 0;  // blue
            for (int c = 0; c < 2; c++)
            {
                int value = texture_data[(height - y - 1) * width + x][c];
                if (value > max_value) value = max_value;
                if (value < 0) value = 0;
                write_data[write_pos++] = value * 255 / max_value;
            }
            write_data[write_pos++] = 255;  //alpha?
        }
    }

    FILE *fid = NULL;
    fopen_s(&fid, filename, "wb");
    if (fid) {
        fwrite(&tgaHeader, sizeof(tgaHeader), 1, fid);
        fwrite(write_data, 1, width * height * 4, fid);
        fclose(fid);
    }

    delete [] write_data;
}

void PictureWriter::SaveRGBATGA(int width, int height, int *data, const char * filename)
{
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
    tgaHeader.bits = 32;
    tgaHeader.descriptor = 0;

    unsigned char *write_data = new unsigned char[width * height * 4];

    int write_pos = 0;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            for (int c = 0; c < 3; c++)
            {
                int value = data[((height - y - 1) * width + x) * 4 + c];
                if (value > 255) value = 255;
                if (value < 0) value = 0;
                write_data[write_pos++] = value;
            }
            write_data[write_pos++] = 255;  //alpha?
        }
    }

    FILE *fid = NULL;
    fopen_s(&fid, filename, "wb");
    if (fid)
    {
        fwrite(&tgaHeader, sizeof(tgaHeader), 1, fid);
        fwrite(write_data, 1, width * height * 4, fid);
        fclose(fid);
    }

    delete [] write_data;
}

bool PictureWriter::LoadTGA(int width, int height, int *data, const char *filename)
{
    TGAHeader tga_header;

    FILE *fid = NULL;
    fopen_s(&fid, filename, "rb");
    if (!fid) return false;

    fread(&tga_header, sizeof(tga_header), 1, fid);
    if (tga_header.width != width) return false;
    if (tga_header.height != height) return false;

    unsigned char *read_data = new unsigned char[width * height * 4];

    int num_read = fread(read_data, 4, width * height, fid);
    if (num_read != width * height) return false;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            for (int c = 0; c < 3; c++)
            {
                int value = read_data[(y * width + x) * 4 + c];
                data[((height - y - 1) * width + x) * 4 + c] = value;
            }
            data[((height - y - 1) * width + x) * 4 + 3] = 127;  // brightness not used!
        }
    }

    delete [] read_data;
    return true;
}
