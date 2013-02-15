#version 120
varying in mat4 parameters;
varying in vec4 color;

void main(void)
{  
	mat4 p = parameters;
	gl_FragColor = vec4(color.rgb, 1.0);
}
