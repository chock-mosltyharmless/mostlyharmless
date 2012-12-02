package de.badlegrunners.braveryrun.graphics;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

/**
 * @author chock
 *
 */
public class MySurfaceView extends GLSurfaceView {
	/**
	 * Creates the surface view that is used so that user input
	 * can be tracked.
	 * 
	 * @param context The device context
	 */
	public MySurfaceView(Context context) {
		super(context);
	}

	/**
	 * Respond to input events from the user.
	 * 
	 * @param event The motion event
	 * @return True if the event was handled
	 */
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		return true;
	}
}
