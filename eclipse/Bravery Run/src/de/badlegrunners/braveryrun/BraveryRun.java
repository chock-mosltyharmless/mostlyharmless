/**
 * Package of the game Bravery Run.
 */
package de.badlegrunners.braveryrun;

import de.badlegrunners.braveryrun.userinterface.Game;
import de.badlegrunners.braveryrun.graphics.RenderMachine;
import de.badlegrunners.braveryrun.graphics.MySurfaceView;

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
	private MySurfaceView mySurfaceView;
	
	/**
	 * The machine that is the wrapper for all the rendering
	 */
	private RenderMachine renderMachine;
		
	/**
	 * The main game user interface and state machine.
	 */
	protected Game game;
	
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
        mySurfaceView = new MySurfaceView(this);
        
        // set our renderer to be the main renderer with the
        // current activity context
        mySurfaceView.setEGLConfigChooser(false);
        renderMachine = new RenderMachine();
        game = new Game(this, renderMachine);
        mySurfaceView.setRenderer(renderMachine);
        setContentView(mySurfaceView);
        
        // Render the view only when there is a change in
        // the drawing area. This is signalled by a requestRender()
        // call to the glSurfaceView.
        //mySurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }
    
    /**
     * Remember to resume the glSurface
     */
    @Override
    protected void onResume() {
    	super.onResume();
    	mySurfaceView.onResume();
    }
    
    /**
     * Also pause the glSurface
     */
    @Override
    protected void onPause() {
    	super.onPause();
    	mySurfaceView.onPause();
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
