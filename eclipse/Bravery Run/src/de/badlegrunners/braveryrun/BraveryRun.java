/**
 * Package of the game Bravery Run.
 */
package de.badlegrunners.braveryrun;

import de.badlegrunners.braveryrun.gamelogic.Party;
import de.badlegrunners.braveryrun.util.DataScanner;
import java.util.Scanner;

import de.badlegrunners.braveryrun.graphics.RenderMachine;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.Window;
import android.view.WindowManager;

/**
 * The main activity of the game that is run when the game is started.
 * 
 * @author chock
 */
public class BraveryRun extends Activity {

	/**
	 * The OpenGL view.
	 * TODO extend this class and move it into braveryrun.graphics so
	 * that I can respond to touch events.
	 */
	private GLSurfaceView glSurfaceView;
	
	/**
	 * Called when the activity is created.
	 * 
	 * @param savedInstanceState TODO I do not know what this is used for
	 */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_bravery_run);
        
        // requesting to turn the title OFF
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        // making it full screen
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
        				     WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        // Initiate OpenGL view and create an instance with
        // this activity
        glSurfaceView = new GLSurfaceView(this);
        
        // set our renderer to be the main renderer with the
        // current activity context
        glSurfaceView.setRenderer(new RenderMachine(this));
        setContentView(glSurfaceView);
        
        // Testing: load dataset
        String dataSet = getResources().getString(R.string.example_dataset);
        Scanner scan = new Scanner(dataSet);
        DataScanner datScan = new DataScanner(scan);
        try
        {
        	datScan.checkToken("Party", "Main");
        	datScan.checkToken("{", "Main");
        	Party party = new Party(scan, 1);
        	datScan.checkToken("}", "Main");
        }
        catch (Exception e)
        {
        	System.out.println(e.getMessage());
        }
    }
    
    /**
     * Remember to resume the glSurface
     */
    @Override
    protected void onResume() {
    	super.onResume();
    	glSurfaceView.onResume();
    }
    
    /**
     * Also pause the glSurface
     */
    @Override
    protected void onPause() {
    	super.onPause();
    	glSurfaceView.onPause();
    }

    /**
     * Called when the options menu is created. TODO: Options menu?
     * 
     * @param menu TODO I do not know how a menu is made. 
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_bravery_run, menu);
        return true;
    }
}
