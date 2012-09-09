#include "StdAfx.h"
#include "PictureWriter.h"

PictureWriter::PictureWriter(void)
{
}

PictureWriter::~PictureWriter(void)
{
}

// saves to a file, generating a good filename...
// This also saves a .tga in order to be able to see a preview.
void PictureWriter::saveFile(CFPHeader *header, BYTE *textureData)
{
	char filename[4096];
	FILE *fid;
	int ID = nextFileID();

	// write cfp
	sprintf(filename, "%d.cfp", ID);
	fid = fopen(filename, "wb");
	if (fid)
	{
		fwrite(header, sizeof(*header), 1, fid);
		fwrite(textureData, 1, header->width * header->height * 3, fid);
		fclose(fid);
		appendFileID(ID);
	}

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
	tgaHeader.width = header->width;
	tgaHeader.height = header->height;
	tgaHeader.bits = 24;
	tgaHeader.descriptor = 0;	

	// write tga [optional]
	sprintf(filename, "%d.tga", ID);
	fid = fopen(filename, "wb");
	if (fid)
	{
		fwrite(&tgaHeader, sizeof(tgaHeader), 1, fid);
		fwrite(textureData, 1, header->width * header->height * 3, fid);
		fclose(fid);
	}
}

// load cfp.files and return an unsued ID.
int PictureWriter::nextFileID()
{
	int maxID = -1;
	char line[4096];

	FILE *fid = fopen(CONFIG_FILE_NAME, "r");
	if (fid)
	{
		int retVal = 1;
		while(retVal != 0 && retVal != EOF)
		{
			retVal = fscanf(fid, "%s\n", line);
			// Remove everything up to the decimal point
			int ptr = 0;
			while (line[ptr] != '.' && line[ptr] != 0) ptr++;
			line[ptr] = 0;
	
			int ID = atoi(line);
			if (ID > maxID) maxID = ID;
		}
		fclose(fid);
	}
	else
	{
		maxID = 0;
	}

	return maxID + 1;
}

// append a file ID to cfp.files
void PictureWriter::appendFileID(int ID)
{
	FILE *fid = fopen(CONFIG_FILE_NAME, "a");
	if (fid)
	{
		fprintf(fid, "%d.cfp\n", ID);
		fclose(fid);
	}
}

