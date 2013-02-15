#version 120
varying out mat4 parameters;
varying out vec4 color;

void main(void)
{
	parameters = gl_ModelViewMatrix;
	color = gl_Color;
	// The position is already totally transformed.
	gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);
}
