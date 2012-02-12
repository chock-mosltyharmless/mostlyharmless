uniform sampler2D Texture0;
varying vec3 objectPosition;
varying mat4 parameters;

void main(void)
{  
	//gl_FragColor = vec4(texture2D(Texture0, objectPosition.xy).xyz, 1.0);
	gl_FragColor = vec4(0.);
}
