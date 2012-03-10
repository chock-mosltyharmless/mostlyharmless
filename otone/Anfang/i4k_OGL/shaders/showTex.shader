varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

void main(void)
{
	vec4 col = texture2D(Texture0, gl_TexCoord[0].xy);
	float alpha = 0.33 * (col.r + col.g + col.b);
	if (alpha > 0.01)
	{
		col = col / alpha;
	}
	//alpha = sqrt(alpha);
	alpha = sqrt(alpha);
	alpha = smoothstep(0.0, 0.7, alpha);
	col = 1.0 - col;
	col = 0.4 * col + vec4(0.6);
	
	gl_FragColor = vec4(color.rgb * col.rgb, min(1.0, alpha * color.a));
}
