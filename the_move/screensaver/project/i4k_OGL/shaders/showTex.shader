varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

void main(void)
{
	float ftime = parameters[0][0];
	ftime *= 3.0;

	float hXPos = 0.5 - 0.75 + 0.5 * ftime;
	float hYPos = 0.5 - 1.5 + 1.0 * ftime;

	float dx = gl_TexCoord[0].x - hXPos;
	float dy = gl_TexCoord[0].y - hYPos;

	float nx = 0.447;
	float ny = 0.8944;

	float dist = abs(dx * nx + dy * ny);
	float amount = 1.0 - smoothstep(.0, 1.0, dist);

	vec4 col = texture2D(Texture0, gl_TexCoord[0].xy);
	gl_FragColor = col * color * vec4(1.0, 1.0, 1.0, 1.0 - dist);
}
