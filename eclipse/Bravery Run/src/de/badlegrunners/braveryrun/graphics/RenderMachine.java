package de.badlegrunners.braveryrun.graphics;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.opengl.GLSurfaceView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 *  OpenGL Custom renderer used with GLSurfaceView 
 */
public class RenderMachine implements GLSurfaceView.Renderer {
   Context context;   // Application's context

   private FloatBuffer vertexBuffer;  // Buffer for vertex-array
  
   private float[] vertices = {  // Vertices of the triangle
       0.0f,  1.0f, 0.0f, // 0. top
       1.0f, -1.0f, 0.0f, // 2. right-bottom
       -1.0f, -1.0f, 0.0f // 1. left-bottom
   };
   
   // Constructor with global application context
   public RenderMachine(Context context) {
      this.context = context;
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
      ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length * 4);
      vbb.order(ByteOrder.nativeOrder()); // Use native byte order
      vertexBuffer = vbb.asFloatBuffer(); // Convert byte buffer to float
      vertexBuffer.put(vertices);         // Copy data into buffer
      vertexBuffer.position(0);           // Rewind
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
   
   // Call back to draw the current frame.
   public void onDrawFrame(GL10 gl) {
	  vertices[0] += 0.1f;
	  vertexBuffer.put(vertices);         // Copy data into buffer
	  vertexBuffer.position(0);           // Rewind
	   
      // Clear color and depth buffers using clear-value set earlier
      gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
     
      // You OpenGL|ES rendering code here
      // Enable vertex-array and define the buffers
      gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
      gl.glVertexPointer(3, GL10.GL_FLOAT, 0, vertexBuffer);
      
      // Draw the primitives via index-array
      //gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_BYTE, indexBuffer);
      gl.glDrawArrays(GL10.GL_TRIANGLES, 0, 3);
      gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
   }
}

