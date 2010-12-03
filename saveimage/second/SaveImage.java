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

public class SaveImage extends Component implements ActionListener, ChangeListener, ItemListener
{
    /* Menu items */
    JMenuBar menuBar;
    JMenu fileMenu;
    JMenuItem loadItem, saveItem, loadPaletteItem, savePaletteItem, exitItem;
    
    /* edit items */
    JComboBox choices, formats;

    /* The image and its attributes */
    BufferedImage original, dithered;

    /* The dithering machine */
    Ditherer ditherer;
    
    /* The sliders for the amount of dithering */
    JSlider noiseSlider, checkerboardSlider, floydSlider, jarvisSlider;
    JSlider luminanceSlider, cyanOrangeSlider, magentaGreenSlider;
    ColorSelector colorSelector;
    Checkbox pixels2x1, color4;
    
    /* The controls for visulization */
    JSlider zoomSlider;
        
    /* Constructor that loads the image*/
    public SaveImage(JFrame frame) {    
        /* initialize ditherer */
        ditherer = new Ditherer();
    
        if (!loadImage()) System.exit(-1);
        
        /* initialize GUI */
        initGUI(frame);
        doDither();
    }

    /* Initialize the graphics controls and the menu */
    void initGUI(JFrame frame)
    {
        /* The menu */
        menuBar = new JMenuBar();
        fileMenu = new JMenu("File");
        menuBar.add(fileMenu);
        loadItem = new JMenuItem("Load Image");
        loadItem.addActionListener(this);
        fileMenu.add(loadItem);        
        saveItem = new JMenuItem("Save Image");
        saveItem.addActionListener(this);
        fileMenu.add(saveItem);
        fileMenu.addSeparator();
        loadPaletteItem = new JMenuItem("Load Palette");
        loadPaletteItem.addActionListener(this);
        fileMenu.add(loadPaletteItem);
        savePaletteItem = new JMenuItem("Save Palette");
        savePaletteItem.addActionListener(this);
        fileMenu.add(savePaletteItem);
        fileMenu.addSeparator();        
        exitItem = new JMenuItem("Exit");
        exitItem.addActionListener(this);
        fileMenu.add(exitItem);
        frame.setJMenuBar(menuBar);

        /* temporary slider values */
        Container panel, superPanel;
        JPanel p2;
        JLabel lab;
        superPanel = new Box(BoxLayout.Y_AXIS);

        /* The display sliders */
        panel = new Box(BoxLayout.Y_AXIS);
        zoomSlider = new JSlider(JSlider.VERTICAL, 1, 8, 1);
        zoomSlider.addChangeListener(this);
        zoomSlider.setMinorTickSpacing(1);
        zoomSlider.setPaintTicks(true);
        lab = new JLabel("Zoom");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(zoomSlider);
        panel.add(p2);
        /* Pack and display stuff */
        frame.add("West", panel);        

        /* The dithering sliders */
        panel = new Box(BoxLayout.X_AXIS);
        noiseSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        noiseSlider.addChangeListener(this);
        noiseSlider.setMinorTickSpacing(10);
        noiseSlider.setPaintTicks(true);
        lab = new JLabel("3 Pixel Ordered");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(noiseSlider);
        panel.add(p2);
        checkerboardSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        checkerboardSlider.addChangeListener(this);
        checkerboardSlider.setMinorTickSpacing(10);
        checkerboardSlider.setPaintTicks(true);
        lab = new JLabel("Checkerboard");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(checkerboardSlider);
        panel.add(p2);                
        floydSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        floydSlider.addChangeListener(this);
        floydSlider.setMinorTickSpacing(10);
        floydSlider.setPaintTicks(true);
        lab = new JLabel("Floyd-Steinberg");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(floydSlider);
        panel.add(p2);
        jarvisSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        jarvisSlider.addChangeListener(this);
        jarvisSlider.setMinorTickSpacing(10);
        jarvisSlider.setPaintTicks(true);
        lab = new JLabel("Jarvis");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(jarvisSlider);
        panel.add(p2);
        superPanel.add(panel);
        panel = new Box(BoxLayout.X_AXIS);
        colorSelector = new ColorSelector(this);
        //colorSelector.addChangeListener(this);
        panel.add(colorSelector);
        superPanel.add(panel);
        
        // The YIQ color sliders
        panel = new Box(BoxLayout.X_AXIS);
        luminanceSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        luminanceSlider.addChangeListener(this);
        luminanceSlider.setMinorTickSpacing(10);
        luminanceSlider.setPaintTicks(true);
        lab = new JLabel("Luminance");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(luminanceSlider);
        panel.add(p2);                
        cyanOrangeSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        cyanOrangeSlider.addChangeListener(this);
        cyanOrangeSlider.setMinorTickSpacing(10);
        cyanOrangeSlider.setPaintTicks(true);
        lab = new JLabel("Cyan Orange");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(cyanOrangeSlider);
        panel.add(p2);     
        magentaGreenSlider = new JSlider(JSlider.HORIZONTAL, 0, 100, 0);
        magentaGreenSlider.addChangeListener(this);
        magentaGreenSlider.setMinorTickSpacing(10);
        magentaGreenSlider.setPaintTicks(true);
        lab = new JLabel("Magenta Green");
        lab.setAlignmentX(Component.CENTER_ALIGNMENT);
        p2 = new JPanel();
        p2.setLayout(new BoxLayout(p2, BoxLayout.Y_AXIS));
        p2.add(lab);
        p2.add(magentaGreenSlider);
        panel.add(p2);
        superPanel.add(panel);
        
        // The parameters checkbox
        panel = new JPanel();
        pixels2x1 = new Checkbox("2x1 Pixels", false);
        pixels2x1.addItemListener(this);
        panel.add(pixels2x1);
        color4 = new Checkbox("4 Colors per block", false);
        color4.addItemListener(this);
        //panel.add(color4);
        superPanel.add(panel);
                
        /* Pack and display stuff */
        frame.add("North", superPanel);
        frame.pack();
    }

    public Dimension getPreferredSize() {
        return new Dimension(2*original.getWidth(null), original.getHeight(null));
    }

    public void paint(Graphics g) {
        doDither();
        int zoom = zoomSlider.getValue();
        int width = original.getWidth(null);
        int height = original.getHeight(null);
        g.drawImage(dithered, 0, 0, width*zoom, height*zoom,
                              0, 0, width, height, null);
        g.drawImage(original, width*zoom, 0, 2*width*zoom, height*zoom,
                              0, 0, width, height, null);
    }

    /* Return the formats sorted alphabetically and in lower case */
    public String[] getFormats() {
        String[] formats = ImageIO.getWriterFormatNames();
        TreeSet<String> formatSet = new TreeSet<String>();
        for (String s : formats) {
            formatSet.add(s.toLowerCase());
        }
        return formatSet.toArray(new String[0]);
    }

    /* The actual dithering is done here */
    void doDither()
    {
        double noiseValue, checkerboardValue, floydValue, jarvisValue;
        noiseValue = (double)noiseSlider.getValue() / 100.0;
        checkerboardValue = (double)checkerboardSlider.getValue() / 100.0;
        floydValue = (double)floydSlider.getValue() / 100.0;
        jarvisValue = (double)jarvisSlider.getValue() / 100.0;
        
        double luminanceValue, cyanOrangeValue, magentaGreenValue;
        luminanceValue = (double)luminanceSlider.getValue() / 100.0;
        cyanOrangeValue = (double)cyanOrangeSlider.getValue() / 100.0;
        magentaGreenValue = (double)magentaGreenSlider.getValue() / 100.0;

        ditherer.setParameters(noiseValue, checkerboardValue, floydValue,
                               jarvisValue, luminanceValue, cyanOrangeValue,
                               magentaGreenValue);
        ditherer.setOptions(pixels2x1.getState(), color4.getState());
        ditherer.doDither(colorSelector);
    }
    
    /* When sliders are dragged... */
    public void stateChanged(ChangeEvent e) {
        repaint();
    }
    
    /* when options are changed */
    public void itemStateChanged(ItemEvent e) {
        repaint();
    }

    /* When the GUI is clicked... */
    public void actionPerformed(ActionEvent e)
    {
        if (e.getSource().equals(exitItem))
        {
            System.exit(0);
        }
        else if (e.getSource().equals(loadItem))
        {
            loadImage();
            repaint();
        }
        else if (e.getSource().equals(saveItem))
        {
            saveImage();
        }
        else if (e.getSource().equals(savePaletteItem))
        {
            savePalette();
        }
        else if (e.getSource().equals(loadPaletteItem))
        {
            loadPalette();
            repaint();
        }        
    };

    boolean loadPalette()
    {
        try {
            // Create a file chooser
            final JFileChooser fc = new JFileChooser();
	        PaletteFilter pf = new PaletteFilter();
            fc.addChoosableFileFilter(pf);
            fc.setAcceptAllFileFilterUsed(true);
            fc.setFileFilter(pf);                     
            int returnVal = fc.showOpenDialog(this);
            if (returnVal != JFileChooser.APPROVE_OPTION)
              {
                throw new IOException("What the fuck?");
              }
            ditherer.loadPalette(fc.getSelectedFile());
            return true;
        } catch (IOException e) {
            System.out.println("Image could not be read");
            
            return false;
        }    
    }
    
    boolean savePalette()
    {
        try
        {
            final JFileChooser fc = new JFileChooser();
	        PaletteFilter pf = new PaletteFilter();
            fc.addChoosableFileFilter(pf);
            fc.setAcceptAllFileFilterUsed(true);
            fc.setFileFilter(pf);
            int returnVal = fc.showSaveDialog(this);
            if (returnVal != JFileChooser.APPROVE_OPTION)
            {
                return false;
            }
            ditherer.savePalette(fc.getSelectedFile());
        }
        catch (IOException e)
        {
            System.out.println("Failed for obvious reasons.");
            return false;
        }
        
        return true;
    }    

    boolean loadImage()
    {
        try {
            // Create a file chooser
            final JFileChooser fc = new JFileChooser();
    	    //Add a custom file filter and disable the default
	        //(Accept All) file filter.
	        ImageFilter imf = new ImageFilter();
            fc.addChoosableFileFilter(imf);
            fc.setAcceptAllFileFilterUsed(true);
            fc.setFileFilter(imf);            
	        //Add custom icons for file types.
            fc.setFileView(new ImageFileView());
	        //Add the preview pane.
            fc.setAccessory(new ImagePreview(fc));            
            
            int returnVal = fc.showOpenDialog(this);
            if (returnVal != JFileChooser.APPROVE_OPTION)
              {
                throw new IOException("What the fuck?");
              }
            original = ImageIO.read(fc.getSelectedFile());
            
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
    
    boolean saveImage()
    {
        try
        {
            final JFileChooser fc = new JFileChooser();
    	    //Add a custom file filter and disable the default
	        //(Accept All) file filter.
	        ImageFilter imf = new ImageFilter();
            fc.addChoosableFileFilter(imf);
            fc.setAcceptAllFileFilterUsed(true);
            fc.setFileFilter(imf);            
	        //Add custom icons for file types.
            fc.setFileView(new ImageFileView());
	        //Add the preview pane.
            fc.setAccessory(new ImagePreview(fc));                        
            
            int returnVal = fc.showSaveDialog(this);
            if (returnVal != JFileChooser.APPROVE_OPTION)
            {
                return false;
            }
            ImageIO.write(dithered, Utils.getExtension(fc.getSelectedFile()),
                          fc.getSelectedFile());
        }
        catch (IOException e)
        {
            System.out.println("Failed for obvious reasons.");
            return false;
        }
        
        return true;
    }

    public static void main(String s[]) {
        JFrame f = new JFrame("Save Image Sample");
        f.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {System.exit(0);}
        });
        SaveImage si = new SaveImage(f);
        f.add("Center", si);   
        f.pack();
        f.setVisible(true);            
    }
}
