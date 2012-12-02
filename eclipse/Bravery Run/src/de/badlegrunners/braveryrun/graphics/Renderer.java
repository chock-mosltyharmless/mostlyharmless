package de.badlegrunners.braveryrun.graphics;

import javax.microedition.khronos.opengles.GL10;

/**
 * A Class that can be used by the render machine in order to
 * render some state of the game.
 * 
 * @author chock
 */
public interface Renderer {
	/**
	 * The call to render the screen depending on the current state.
	 * 
	 * @param gl The openGL1.0 interface that is used for rendering
	 */
	public void render(GL10 gl);
}
