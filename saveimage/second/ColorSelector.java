import javax.swing.SwingUtilities;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.BorderFactory;
import javax.swing.event.*;
import java.awt.Color;
import java.awt.GradientPaint;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseMotionAdapter; 

class ColorSelector extends JPanel implements MouseListener, MouseMotionListener
{
    // color data
    static final int maxNumColors = 32;
    public int numColors;    
    public int color[];    
    
    // editing data
    int selected; // currently selected color, -1 if none.
    ChangeListener changeListener;

    public ColorSelector(ChangeListener cl)
    {
        numColors = 4;
        color = new int[maxNumColors];
        color[0] = 0;
        color[1] = 85;
        color[2] = 170;
        color[3] = 255;
        selected = -1;
        changeListener = cl;
    
        setBorder(BorderFactory.createLineBorder(Color.black));
        
        addMouseListener(this);
        addMouseMotionListener(this);
    }

    int getColor(int xPos)
    {
        int col;
        int width = getWidth()-2;
        
        col = (xPos-1)*255/width;
        if (col < 0) col = 0;
        if (col > 255) col = 255;
        
        return col;
    }
        
    /* Mouse movement */
    public void mouseDragged(MouseEvent e)
    {
        moveMouse(e);
    }
    
    public void mouseMoved(MouseEvent e)
    {        
        moveMouse(e);
    }
    
    void moveMouse(MouseEvent e)
    {    
        if (selected >= 0 &&
            (e.getModifiersEx() & MouseEvent.BUTTON1_DOWN_MASK) == MouseEvent.BUTTON1_DOWN_MASK)
        {
            color[selected] = getColor(e.getX());
            changeListener.stateChanged(new ChangeEvent("Nothing"));
            repaint();
        }
    }
    
    public void mouseClicked(MouseEvent e)
    {    
    }

    public void mouseEntered(MouseEvent e)
    {
        int width = getWidth()-2;
        int height = getHeight();        
    
        if ((e.getModifiersEx() & MouseEvent.BUTTON1_DOWN_MASK) == MouseEvent.BUTTON1_DOWN_MASK)
        {
            if (numColors < maxNumColors)
            {
                color[numColors] = getColor(e.getX());
                selected = numColors;
                numColors++;        
                changeListener.stateChanged(new ChangeEvent("Nothing"));                
                repaint();
            }
        }
    }
    
    public void mouseExited(MouseEvent e)
    {
        if ((e.getModifiersEx() & MouseEvent.BUTTON1_DOWN_MASK) != MouseEvent.BUTTON1_DOWN_MASK)
        {
            selected = -1;
        }
    
        if (selected >= 0 && numColors > 2)
        {
            // copy rest stuff
            for (int i = selected; i < numColors - 1; i++)
            {
                color[i] = color[i + 1];
            }
            numColors--;
            selected = -1;
            changeListener.stateChanged(new ChangeEvent("Nothing"));                            
            repaint();
        }        
    }
    
    public void mousePressed(MouseEvent e)
    {
        int curCol = getColor(e.getX());
        
        selected = 0;
        int bestDist = Math.abs(curCol - color[selected]);
        
        for (int i = 1; i < numColors; i++)
        {
            if (Math.abs(curCol - color[i]) < bestDist)
            {
                selected = i;
                bestDist = Math.abs(curCol - color[i]);
            }
        }
        
        color[selected] = curCol;
        changeListener.stateChanged(new ChangeEvent("Nothing"));                
        repaint();
    }
    
    public void mouseReleased(MouseEvent e)
    {
        selected = -1;
    }

    
    /* painting and stuff */    
    public Dimension getPreferredSize()
    {
        return new Dimension(512, 30);
    }
    
    public void paintComponent(Graphics g)
    {
        int width = getWidth() - 2;
        int height = getHeight();
    
        super.paintComponent(g);
        
        Graphics2D g2 = (Graphics2D)g;
        
        // Draw Background
        g2.setPaint(new GradientPaint(0.0f, height*0.5f, Color.black,
                                      width+2.0f, height*0.5f, Color.white));
        g.fillRect(0, 0, width+2, height);
        
        // Draw Lines
        for (int i = 0; i < numColors; i++)
        {        
            g.setColor(Color.black);
            g.drawLine(color[i]*width/255, 0, color[i]*width/255, height);
            g.drawLine(color[i]*width/255+2, 0, color[i]*width/255+2, height);
            g.setColor(Color.white);
            g.drawLine(color[i]*width/255+1, 0, color[i]*width/255+1, height);
        }
    }
}
