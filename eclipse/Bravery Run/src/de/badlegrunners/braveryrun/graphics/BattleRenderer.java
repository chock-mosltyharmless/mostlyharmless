package de.badlegrunners.braveryrun.graphics;

import java.nio.IntBuffer;

import de.badlegrunners.braveryrun.userinterface.*;
import de.badlegrunners.braveryrun.graphics.RenderMachine;

import javax.microedition.khronos.opengles.GL10;

/**
 * Rendering of a battle. This class is tightly bound to a battle
 * class.
 * This class does rendering specific for battles and handles user
 * input specific for battles.
 * 
 * @author chock
 */
public class BattleRenderer implements Renderer {
	/**
	 * TODO: To be removed.
	 */
	private int testVertices[] = {  // Vertices of the triangle
			0<<16,  1<<16, 0<<16, // 0. top
			1<<16, -1<<16, 0<<16, // 2. right-bottom
			-1<<16, -1<<16, 0<<16 // 1. left-bottom
	};	
	
	/**
	 * The RenderMachine used for rendering. The rendermachine
	 * holds vertex buffers and the like.
	 */
	private final RenderMachine renderMachine;
	
	/**
	 * The user interface class that is user for the interaction
	 * between graphics, user input and the game logic.
	 */
	private final Battle battleUI;
	
	/**
	 * Create the renderer for the battlefield.
	 * This is not good.
	 * 
	 * @param battleUI The battle user interface that shall be
	 * rendered.
	 */
	public BattleRenderer(Battle battleUI, RenderMachine renderMachine) {
		this.battleUI = battleUI;
		this.renderMachine = renderMachine;
	}
	
	/**
	 * Renders the battle using the state information of the battle UI.
	 * 
	 * @param gl The openGL1.0 interface that is used for rendering
	 */
	public void render(GL10 gl) {
		IntBuffer vertexBuffer = renderMachine.getVertexBuffer();
		testVertices[0] += 1<<13;
		vertexBuffer.put(testVertices);         // Copy data into buffer
		vertexBuffer.position(0);           // Rewind
	  
		// Clear color and depth buffers using clear-value set earlier
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
     
		// You OpenGL|ES rendering code here
		// Enable vertex-array and define the buffers
		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
		gl.glVertexPointer(3, GL10.GL_FIXED, 0, vertexBuffer);
      
		// Draw the primitives via index-array
		//gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_BYTE, indexBuffer);
		gl.glDrawArrays(GL10.GL_TRIANGLES, 0, 3);
		// Why disable?
		//gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
	}
}
