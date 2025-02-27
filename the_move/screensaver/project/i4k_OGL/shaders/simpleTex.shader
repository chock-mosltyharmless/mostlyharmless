varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

void main(void)
{
	vec4 col = texture2D(Texture0, gl_TexCoord[0].xy);
	gl_FragColor = col * color;
}
