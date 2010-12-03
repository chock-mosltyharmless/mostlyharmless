import java.util.Random;
import java.awt.image.*;
import java.io.*;

public class Ditherer
{
    /* The RGB to YIQ matrix */
    double RGB2YIQ[][] =
    {
        {0.144, 0.587, 0.299},
        {-0.321, -0.275, 0.596},
        {0.311, -0.523, 0.212}
    };
    /* The YIQ to RGB matrix */
    double YIQ2RGB[][] =
    {
        {0.971, -1.076, 1.655},
        {0.971, -0.239, -0.697},
        {0.971, 0.988, 0.570}
    };

    /* The C64 palette (static) */
    double RGBpalette[][] =
    {
        {0x00, 0x00, 0x00},
        {0xfc, 0xfc, 0xfc},
        {0x8b, 0x1f, 0x00},
        {0x65, 0xCD, 0xA8},
        {0xA7, 0x3B, 0x9F},
        {0x4F, 0xB3, 0x17},
        {0x1B, 0x0D, 0x93},
        {0xF3, 0xEB, 0x5B},
        {0xA3, 0x47, 0x00},
        {0x3F, 0x1C, 0x00},
        {0xCB, 0x7B, 0x6F},
        {0x45, 0x44, 0x44},
        {0x83, 0x83, 0x83},
        {0x97, 0xFF, 0x97},
        {0x4F, 0x93, 0xD3},
        {0xBB, 0xBB, 0xBB},
    };

    /* The YIQ palette (dynamic) */
    double palette[][];

    /* The static color error weight function for Y, I and Q*/
    double errorWeight[] = {1.0, 0.375, 0.15};

    /* The error diffusion matrices */
    /*double floydMatrix[][] =
    {
        {0.0,      0.0,      0.0,      7.0/16.0, 0.0},
        {0.0,      3.0/16.0, 5.0/16.0, 1.0/16.0, 0.0},
        {0.0,      0.0,      0.0,      0.0,      0.0}
    };*/
    double floydMatrix[][] =
    {
        {0.0,      0.0,      1.0/28.0, 0.0/16.0, 0.0},
        {0.0,      2.0/28.0, 4.0/28.0, 2.0/28.0, 0.0},
        {1.0/28.0, 4.0/28.0, 0.0,      4.0/28.0, 1.0/28.0},
        {0.0,      2.0/28.0, 4.0/28.0, 2.0/28.0, 0.0},
        {0.0,      0.0,      1.0/28.0, 0.0,      0.0}
    };
    double jarvisMatrix[][] =
    {
        {0.0,      0.0,      0.0,      0.0/16.0, 0.0},
        {0.0,      0.0,      0.0,      0.0/16.0, 0.0},
        {0.0,      0.0,      0.0,      7.0/48.0, 5.0/48.0},
        {3.0/48.0, 5.0/48.0, 7.0/48.0, 5.0/48.0, 3.0/48.0},
        {1.0/48.0, 3.0/48.0, 5.0/48.0, 3.0/48.0, 1.0/48.0}
    };

    /* The checkerboard matrix */
    double checkerboardMatrix[][] =
    {
        {-32.0, 32.0},
        {32.0, -32.0}
    };

    double orderedMatrix[][] =
    {
        {16.0, 32.0, 0.0},
        {-24.0, -32.0, -8.0},
        {8.0, -16.0, 24.0}
    };
    double orderedMatrix2x1[][] =
    {
        {-22.0, 11.0},
        {33.0, -33.0},
        {-11.0, 22.0}
    };

    // Parameters for dithering
    private double noiseValue, checkerboardValue, floydValue, jarvisValue;
    private boolean pixel2x1, color4;

    // Temporary dithering data
    BufferedImage original, dithered;
    double originalArr[][][];
    double ditheredArr[][][];

    // Set the image that is as original and the output image
    public void setImage(BufferedImage orig, BufferedImage dith)
    {
        original = orig;
        dithered = dith;
        originalArr = new double [original.getHeight(null)][original.getWidth(null)][3];
        ditheredArr = new double [original.getHeight(null)][original.getWidth(null)][3];
        Raster data = original.getData();
        double dArray[] = new double[3];
        for (int y = 0; y < original.getHeight(null); y++)
        {
            for (int x = 0; x < original.getWidth(null); x++)
            {
                data.getPixel(x, y, dArray);
//                originalArr[y][x] = 0.11 * dArray[0] +
//                                    0.59 * dArray[1] +
//                                    0.30 * dArray[2];
                convertColor(dArray, originalArr[y][x], RGB2YIQ);
            }
        }
    }

    // Set the parameters
    public void setParameters(double noise, double checkerboard,
                              double floyd, double jarvis,
                              double luminance, double cyanOrange, double magentaGreen)
    {
        noiseValue = noise;
        checkerboardValue = checkerboard;
        floydValue = floyd;
        jarvisValue = jarvis;

        if (jarvisValue + floydValue > 1.0)
        {
            double divisor = jarvisValue + floydValue;
            jarvisValue /= divisor;
            floydValue /= divisor;
        }

        errorWeight[0] = luminance;
        errorWeight[1] = cyanOrange;
        errorWeight[2] = magentaGreen;

        /* create YIQ palette */
        palette = new double[RGBpalette.length][3];
        for (int i = 0; i < RGBpalette.length; i++)
        {
            convertColor(RGBpalette[i], palette[i], RGB2YIQ);
        }
    }

    public void savePalette(File file) throws IOException
    {
        BufferedWriter out = new BufferedWriter(new FileWriter(file));
        for (int i = 0; i < palette.length; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                out.write("0x");
                out.write(Integer.toHexString((int)RGBpalette[i][c]));
                out.write(" ");
            }
            out.newLine();
        }
        out.close();
    }

    // Set the options
    public void setOptions(boolean pix2x1, boolean col4)
    {
        pixel2x1 = pix2x1;
        color4 = col4;
    }

    public void loadPalette(File file) throws IOException
    {
        int numEntries = 0;
        boolean finished;

        /* check how many palette entries we have */
        BufferedReader in = new BufferedReader(new FileReader(file));
        finished = false;
        while (!finished)
        {
            finished = true;
            String read = in.readLine();
            if (read != null)
            {
                read = read.trim();
                finished = false;
                if (read.length() > 0)
                {
                    if (!read.startsWith("#"))
                    {
                        numEntries++;
                    }
                }
            }
        }
        in.close();

        double [][] pal = new double[numEntries][3];

        /* read palette entries */
        in = new BufferedReader(new FileReader(file));
        finished = false;
        int entry = 0;
        while (!finished)
        {
            finished = true;
            String read = in.readLine();
            if (read != null)
            {
                read = read.trim();
                finished = false;
                if (read.length() > 0)
                {
                    if (!read.startsWith("#"))
                    {
                        int rPos = read.length();
                        int lPos = read.lastIndexOf(' ');
                        pal[entry][2] = Integer.decode(read.substring(lPos+1, rPos)).doubleValue();
                        rPos = lPos;
                        lPos = read.substring(0, rPos).lastIndexOf(' ');
                        pal[entry][1] = Integer.decode(read.substring(lPos+1, rPos)).doubleValue();
                        rPos = lPos;
                        lPos = read.substring(0, rPos).lastIndexOf(' ');
                        pal[entry][0] = Integer.decode(read.substring(lPos+1, rPos)).doubleValue();
                        entry++;
                    }
                }
            }
        }
        in.close();

        RGBpalette = pal;
    }

    /* Calculate palette entries for 8x8 (or 4x8) blocks */
    private double [][][][] getPalettes(int colorsPerBlock)
    {
        int width;
        int height = (originalArr.length + 7) / 8;
        if (pixel2x1)
        {
            width = (originalArr[0].length + 3) / 4;
        }
        else
        {
            width = (originalArr[0].length + 3) / 4;
        }

        double [][][][] pals = new double[height][width][][];

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double [][] curPal = new double[colorsPerBlock][];
                double [][] bestPal = new double[colorsPerBlock][];
                double bestDistance;

                for (int color = 0; color < colorsPerBlock; color++)
                {
                    curPal[color] = palette[color];
                    bestPal[color] = curPal[color];
                }
                bestDistance = 1000000.0;

                for (int iteration = 0; iteration < 2 && bestDistance > 0; iteration++)
                {
                    for (int color = 0; color < colorsPerBlock && bestDistance > 0; color++)
                    {
                        for (int c = 0; c < colorsPerBlock; c++)
                        {
                            curPal[c] = bestPal[c];
                        }
                        for (int newCol = 0; newCol < palette.length && bestDistance > 0; newCol++)
                        {
                            /* exchange color for test */
                            curPal[color] = palette[newCol];

                            /* check distance (TODO: This is not optimal for 2x1 pixel */
                            double distance = 0.0;
                            for (int yp = 0; yp < 8 && y*8+yp < ditheredArr.length; yp++)
                            {
                                for (int xp = 0; xp < 8 && x*8+xp < ditheredArr[0].length; xp++)
                                {
                                    double [] origVal = ditheredArr[y*8+yp][x*8+xp];
                                    double [] newVal = findBestPaletteEntry(origVal, curPal);
                                    for (int c = 0; c < 3; c++)
                                    {
                                        distance += Math.abs(origVal[c] - newVal[c]);
                                    }
                                }
                            }

                            /* Check if we are better than optimum */
                            if (distance < bestDistance)
                            {
                                bestDistance = distance;
                                for (int c = 0; c < colorsPerBlock; c++)
                                {
                                    bestPal[c] = curPal[c];
                                }
                            }
                        }
                    }
                }

                pals[y][x] = bestPal;
            }
        }

        return pals;
    }

    /* Colored dithering (using static palette and error weighting) */
    public void doColorDither()
    {
        Random random = new Random();

        double [][][][] pals = null;

        /* Generate the actual error diffusion matrix */
        double [][] errorDiff = new double[5][5];
        for (int y = 0; y < 5; y++)
        {
            for (int x = 0; x < 5; x++)
            {
                errorDiff[y][x] = floydValue * floydMatrix[y][x] +
                                  jarvisValue * jarvisMatrix[y][x];
            }
        }
        /* Adjust error diffusion for 2x1 pixels */
        if (pixel2x1)
        {
            errorDiff[2][3] += errorDiff[2][2];
            errorDiff[2][3] = 0.0;
        }

            /* copy original to dithered */
            for (int y = 0; y < originalArr.length; y++)
            {
                for (int x = 0 ; x < originalArr[0].length; x++)
                {
                    for (int c = 0; c < originalArr[0][0].length; c++)
                    {
                        ditheredArr[y][x][c] = originalArr[y][x][c];
                            /*noiseValue * 64.0 * (random.nextDouble() - 0.5);*/
                    }
                    if (pixel2x1)
                    {
                        ditheredArr[y][x][0] += checkerboardValue * checkerboardMatrix[y&1][(x>>1)&1];
                        ditheredArr[y][x][0] += noiseValue * orderedMatrix2x1[y%3][(x>>1)&1];
                    }
                    else
                    {
                        ditheredArr[y][x][0] += checkerboardValue * checkerboardMatrix[y&1][x&1];
                        ditheredArr[y][x][0] += noiseValue * orderedMatrix[y%3][x%3];
                    }
                }
            }

        for (int iteration = 0; iteration < 4; iteration++)
        {
            /* For now, I just find the best match in the palette */
            double [] error = new double [3];
            double [] rgbColor = new double [3];
            int y = 0;
            if ((iteration&1) != 0)
            {
                y = originalArr.length-1;
            }
            for (; y < originalArr.length && y >= 0; y = y+1-2*(iteration&1))
            {
                if (pixel2x1)
                {
                    for (int x = 0; x < originalArr[0].length-1; x+= 2)
                    {
                        double [] origValL = ditheredArr[y][x];
                        double [] origValR = ditheredArr[y][x+1];
                        for (int c = 0; c < 3; c++)
                        {
                            rgbColor[c] = 0.5 * (origValL[c] + origValR[c]);
                        }
                        double [] newVal;
                        if (color4 && iteration > 0)
                        {
                            newVal = findBestPaletteEntry(rgbColor, pals[y/8][x/8]);
                        }
                        else
                        {
                            newVal = findBestPaletteEntry(rgbColor, palette);
                        }

                        for (int c = 0; c < 3; c++)
                        {
                            error[c] = rgbColor[c] - newVal[c];
                            if (error[c] < -127) error[c] = -127;
                            if (error[c] > 127) error[c] = 127;
                            ditheredArr[y][x][c] = newVal[c];
                            ditheredArr[y][x+1][c] = newVal[c];
                        }

                        for (int ym = 0; ym < 3; ym++)
                        {
                            for (int xm = -2; xm < 3; xm++)
                            {
                                if ((y + ym < originalArr.length) &&
                                    (x + xm >= 0) &&
                                    (x + xm < originalArr[0].length))
                                {
                                    for (int c = 0; c < 3; c++)
                                    {
                                        ditheredArr[y + ym][x + xm][c] +=
                                            error[c] * errorDiff[ym][xm+2];
                                    }
                                }
                            }
                        }

                        /* write pixel. This should be changed to whole area */
                        convertColor(newVal, rgbColor, YIQ2RGB);
                        dithered.setRGB(x, y, (int)rgbColor[2] |
                                              ((int)rgbColor[1] << 8) |
                                              ((int)rgbColor[0] << 16));
                        dithered.setRGB(x+1, y, (int)rgbColor[2] |
                                              ((int)rgbColor[1] << 8) |
                                              ((int)rgbColor[0] << 16));
                    }
                }
                else
                {
                    int x= 0;
                    if ((iteration & 1) != 0)
                    {
                        x = originalArr[0].length - 1;
                    }
                    for (;x < originalArr[0].length && x >= 0; x = x + 1 - 2*(iteration&1))
                    {
                        double [] origVal = ditheredArr[y][x];
                        double [] newVal;
                        if (color4 && iteration > 0)
                        {
                            newVal = findBestPaletteEntry(origVal, pals[y/8][x/8]);
                        }
                        else
                        {
                            newVal = findBestPaletteEntry(origVal, palette);
                        }
                        for (int c = 0; c < 3; c++)
                        {
                            error[c] = origVal[c] - newVal[c];
                            if (error[c] < -127) error[c] = -127;
                            if (error[c] > 127) error[c] = 127;
                            ditheredArr[y][x][c] = newVal[c];
                        }

                        for (int ym = -2; ym < 3; ym++)
                        {
                            for (int xm = -2; xm < 3; xm++)
                            {
                                if ((y + ym >= 0) &&
                                    (y + ym < originalArr.length) &&
                                    (x + xm >= 0) &&
                                    (x + xm < originalArr[0].length))
                                {
                                    for (int c = 0; c < 3; c++)
                                    {
                                        ditheredArr[y + ym][x + xm][c] +=
                                            1.1 * error[c] * errorDiff[ym+2][xm+2];
                                    }
                                }
                            }
                        }

                        /* write pixel. This should be changed to whole area */
                        convertColor(newVal, rgbColor, YIQ2RGB);
                        dithered.setRGB(x, y, (int)rgbColor[2] |
                                              ((int)rgbColor[1] << 8) |
                                              ((int)rgbColor[0] << 16));
                    }
                }
            }

            /* calculate 4-way palette */
            if (color4 && iteration == 0)
            {
                pals = getPalettes(Math.min(palette.length, 4));
            }
        }
    }

    // The dithering call
    // monochrome
    public void doDither(ColorSelector colorSelector)
    {
        doColorDither();
    }

    public void doBWDither(ColorSelector colorSelector)
    {
        Random random = new Random();

        /* copy original to dithered */
        for (int y = 0; y < originalArr.length; y++)
        {
            for (int x = 0 ; x < originalArr[0].length; x++)
            {
                ditheredArr[y][x][0] = originalArr[y][x][0] +
                    checkerboardValue * checkerboardMatrix[y&1][x&1] +
                    noiseValue * 64.0 * (random.nextDouble() - 0.5);
            }
        }

        /* Generate the actual error diffusion matrix */
        double [][] errorDiff = new double[3][5];
        for (int y = 0; y < 3; y++)
        {
            for (int x = 0; x < 5; x++)
            {
                errorDiff[y][x] = floydValue * floydMatrix[y][x] +
                                  jarvisValue * jarvisMatrix[y][x];
            }
        }

        /* For now, I just find the best match in the palette */
        for (int y = 0; y < originalArr.length; y++)
        {
            for (int x = 0 ; x < originalArr[0].length; x++)
            {
                double origVal = ditheredArr[y][x][0];
                double newVal = findBestMatch(origVal, colorSelector);
                double error = origVal - newVal;

                if (error > 100) error = ((error-100) * 0.1)+100;
                if (error > 50) error = ((error-50) * 0.5)+50;

                if (error < -255) error = -255;
                if (error > 255) error = 255;
                ditheredArr[y][x][0] = newVal;

                for (int ym = 0; ym < 3; ym++)
                {
                    for (int xm = -2; xm < 3; xm++)
                    {
                        if ((y + ym < originalArr.length) &&
                            (x + xm >= 0) &&
                            (x + xm < originalArr[0].length))
                        {
                            ditheredArr[y + ym][x + xm][0] +=
                                error * errorDiff[ym][xm+2];
                        }
                    }
                }

                /* write pixel. This should be changed to whole area */
                int intVal = (int)newVal;
                dithered.setRGB(x, y, intVal | (intVal << 8) | (intVal << 16));
            }
        }
    }

    double [] findBestPaletteEntry(double [] color, double [][] palette)
    {
        double [] bestMatch = palette[0];
        double bestDistance = 1000000.0;

        for (int entry = 0; entry < palette.length; entry++)
        {
            double distance =
                errorWeight[0] * Math.abs(color[0] - palette[entry][0]) +
                errorWeight[1] * Math.abs(color[1] - palette[entry][1]) +
                errorWeight[2] * Math.abs(color[2] - palette[entry][2]);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMatch = palette[entry];
            }
        }

        return bestMatch;
    }

    double findBestMatch(double color, ColorSelector colorSelector)
    {
        double bestMatch = 0;
        double bestDistance = 10000.0;

        for (int i = 0; i < colorSelector.numColors; i++)
        {
            double distance = Math.abs(color - colorSelector.color[i]);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMatch = colorSelector.color[i];
            }
        }

        return bestMatch;
    }

    void convertColor(double input[], double output[], double matrix[][])
    {
        for (int i = 0; i < 3; i++)
        {
            output[i] = 0.0;
            for (int j = 0; j < 3; j++)
            {
                output[i] += input[j] * matrix[i][j];
            }
        }
    }
}
