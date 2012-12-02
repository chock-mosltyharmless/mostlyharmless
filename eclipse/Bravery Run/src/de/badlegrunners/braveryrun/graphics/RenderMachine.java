package de.badlegrunners.braveryrun.graphics;

import de.badlegrunners.braveryrun.graphics.Renderer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.opengl.GLSurfaceView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

/**
 *  OpenGL Custom renderer used with GLSurfaceView 
 */
public class RenderMachine implements GLSurfaceView.Renderer {
	/**
	 * The rendering class that is used to display the current
	 * state. If the screen needs to be updated, this class is called.
	 * The currently active class for rendering has to be set
	 * by the user interface.
	 */
	Renderer renderEngine;
	
	/**
	 * The maximum number of triangles that can be drawn.
	 */
	public final int MAX_NUM_TRIANGLES = 1000;
	
	/**
	 * The maximum number of vertices that are allocated for the
	 * vertex buffer.
	 */
	public final int MAX_NUM_VERTICES = MAX_NUM_TRIANGLES * 3;
	
	/**
	 * The directly mapped integer buffer that holds the vertex data.
	 * Vertex data is stored in 16.16 fixed point format.
	 */
	private IntBuffer vertexBuffer;
  
	/**
	 * A buffer of 16.16 fixed point integers that can be used
	 * as the vertex array, as it already has the correct size.
	 * It does not have to be used, though.
	 * 
	 * You should use this in order to avoid garbage collection on
	 * often created and released buffers.
	 */
	private int vertices[]; 
	
	/**
	 * @return vertexBuffer object used for rendering vertices.
	 */
	public IntBuffer getVertexBuffer() {
		return vertexBuffer;
	}
	
	/**
	 * @return An array of ints that can be used to map vertices to
	 * the vertex buffer.
	 */
	public int[] getVertices() {
		return vertices;
	}
	
	/**
	 * @param renderEngine The game state specific engine used for
	 * 		  rendering.
	 */
	public void setRenderEngine(Renderer renderEngine) {
		this.renderEngine = renderEngine;
	}
	   
	/**
	 * Creates the render machine.
	 */
	public RenderMachine() {
	}

	// Call back when the surface is first created or re-created
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set color's clear-value to black
		gl.glDisable(GL10.GL_DEPTH_TEST);   // Enables depth-buffer for hidden surface removal
		gl.glShadeModel(GL10.GL_SMOOTH);   // Enable smooth shading of color
		gl.glDisable(GL10.GL_LIGHTING);
		gl.glDisable(GL10.GL_TEXTURE_2D);
		gl.glDisable(GL10.GL_CULL_FACE);
		  
		// You OpenGL|ES initialization code here
		// Setup vertex-array buffer. Vertices in float. A float has 4 bytes.
		ByteBuffer vbb = ByteBuffer.allocateDirect(MAX_NUM_VERTICES * 4);
		vbb.order(ByteOrder.nativeOrder()); // Use native byte order
		vertexBuffer = vbb.asIntBuffer(); // Convert byte buffer to float
	}
   
	// Call back after onSurfaceCreated() or whenever the window's size changes
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		// Set the viewport (display area) to cover the entire window
		gl.glViewport(0, 0, width, height);
  
		// Setup perspective projection, with aspect ratio matches viewport
		gl.glMatrixMode(GL10.GL_PROJECTION); // Select projection matrix
		gl.glLoadIdentity();                 // Reset projection matrix
		// Use orthogonal projection
		gl.glOrthof(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f);
  
		gl.glMatrixMode(GL10.GL_MODELVIEW);  // Select model-view matrix
		gl.glLoadIdentity();                 // Reset  
	}
   
	/**
	 *  Call back to draw the current frame.
	 *  This method calls the game state specific render engine.
	 *  
	 *  @param gl The openGL 1.0 render interface
	 */
	public void onDrawFrame(GL10 gl) {
		renderEngine.render(gl);
	}
}

