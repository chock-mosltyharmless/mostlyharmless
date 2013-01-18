#version 120
varying out vec3 objectPosition;
varying out mat4 parameters;
varying out vec4 color;

void main(void)
{
	parameters = gl_ModelViewMatrix;
	objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);
	color = gl_Color;
	gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);
}
