#version 120 
varying vec3 objectPosition;
varying mat4 parameters;

void main(void)
{
   parameters = gl_ModelViewMatrix;
   objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);
   gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);
}