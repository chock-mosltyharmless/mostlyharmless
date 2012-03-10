varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

void main(void)
{
	//gl_FragColor = vec4(color);
	vec4 tex = texture2D(Texture0, objectPosition.xy * 1.5);
	float mixer = parameters[1][0];
	gl_FragColor = (1.0 - mixer) * vec4(0.95, 1.0, 0.9, 1.0) + mixer * tex;
}
