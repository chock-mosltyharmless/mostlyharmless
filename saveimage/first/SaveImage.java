/*
 * Copyright (c) 1995 - 2008 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of Sun Microsystems nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


import java.io.*;
import java.util.TreeSet;
import java.util.Random;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.AffineTransform;
import java.awt.image.*;
import javax.imageio.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.filechooser.*;

public class SaveImage
{
    /* The image and its attributes */
    BufferedImage original, dithered;

    /* The dithering machine */
    Ditherer ditherer;

    double noiseValue, checkerboardValue, floydValue;
    double luminanceValue, cyanOrangeValue, magentaGreenValue;
    boolean pixels2x1;

    /* Constructor that loads the image*/
    public SaveImage() {
        /* initialize ditherer */
        ditherer = new Ditherer();
    }

    /*
    public void paint(Graphics g) {
        doDither();
        int zoom = zoomSlider.getValue();
        int width = original.getWidth(null);
        int height = original.getHeight(null);
        g.drawImage(dithered, 0, 0, width*zoom, height*zoom,
                              0, 0, width, height, null);
        g.drawImage(original, width*zoom, 0, 2*width*zoom, height*zoom,
                              0, 0, width, height, null);
    }*/

    /* The actual dithering is done here */
    void doDither()
    {
        double jarvisValue;
        jarvisValue = 0.0;

        ditherer.setParameters(noiseValue, checkerboardValue, floydValue,
                               jarvisValue, luminanceValue, cyanOrangeValue,
                               magentaGreenValue);
        //ditherer.setOptions(pixels2x1.getState(), color4.getState());
        ditherer.setOptions(pixels2x1, false);
        ditherer.doDither();
    }

    boolean loadPalette(String filename)
    {
        try {
            // Create a file chooser
            File file = new File(filename);
            ditherer.loadPalette(file);
            return true;
        } catch (IOException e) {
            System.out.println("Image could not be read");

            return false;
        }
    }

    boolean loadImage(String filename)
    {
        try {
            File file = new File(filename);
            original = ImageIO.read(file);

            if (original.getType() != BufferedImage.TYPE_INT_RGB) {
                BufferedImage bi2 =
                    new BufferedImage(original.getWidth(null), original.getHeight(null),
                                      BufferedImage.TYPE_INT_RGB);
                Graphics big = bi2.getGraphics();
                big.drawImage(original, 0, 0, null);
                original = bi2;
            }
            dithered = new BufferedImage(original.getWidth(null), original.getHeight(null),
                                         BufferedImage.TYPE_INT_RGB);

            /* Create the monochrome double array */
            ditherer.setImage(original, dithered);
            return true;
        } catch (IOException e) {
            System.out.println("Image could not be read");

            return false;
        }
    }

    boolean saveImage(String filename)
    {
        try
        {
            File file = new File(filename);
            ImageIO.write(dithered, Utils.getExtension(file),
                          file);
        }
        catch (IOException e)
        {
            System.out.println("Failed for obvious reasons.");
            return false;
        }

        return true;
    }

    public static void main(String s[]) {
        SaveImage si = new SaveImage();

        /* double noiseValue, checkerboardValue, floydValue;
           double luminanceValue, cyanOrangeValue, magentaGreenValue;
           boolean pixels2x1;*/

        if (s.length != 10)
        {
            System.out.println("java SaveImage ORDERED CHECKERBOARD FLOYD LUMINANCE CYAN MAGENTA 2X1 PALETTE INPUT OUTPUT");
            System.exit(-1);
        }

        si.noiseValue = Double.valueOf(s[0]).doubleValue();
        si.checkerboardValue = Double.valueOf(s[1]).doubleValue();
        si.floydValue = Double.valueOf(s[2]).doubleValue();
        si.luminanceValue = Double.valueOf(s[3]).doubleValue();
        si.cyanOrangeValue = Double.valueOf(s[4]).doubleValue();
        si.magentaGreenValue = Double.valueOf(s[5]).doubleValue();
        if (Double.valueOf(s[6]).doubleValue() > 0.1)
        {
            si.pixels2x1 = true;
        }
        else
        {
            si.pixels2x1 = false;
        }
        si.loadPalette(s[7]);
        si.loadImage(s[8]);
        si.doDither();
        si.saveImage(s[9]);

    }
}
