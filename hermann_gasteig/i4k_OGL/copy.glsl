uniform sampler2D Texture0; 
varying vec3 objectPosition; 
varying mat4 parameters; 
 
void main(void) 
{   
	float fTime0_X = parameters[0][0];
   
	// mode 0: Normal copy mode
	// mode 0.2: horizontal blur (including color reduction)
	// mode 0.4: vertical blur
	float mode = parameters[0][1]; 
   
	if (mode < 0.1)
	{
		// Normal print to front
		vec3 noisePos = objectPosition + fTime0_X; 
		vec2 noiseVal; 
		noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453); 
		noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453); 
		gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.02; 
	}
	else
	{
		// how far I go left/right in one step (I also need to shift left/right for additional blur?)
		float stepSize = parameters[0][2];
		float correctionSize = parameters[0][3];

		if (mode < 0.3)
		{
			// horizontal blur (including color reduction)
			vec2 startPos = 0.5 * objectPosition.xy + 0.5 - vec2(4.5*stepSize, -0.5*correctionSize);
			gl_FragColor = vec4(0.0);
			for (int i = 0; i < 9; i++)
			{
				gl_FragColor += 0.2 * max(texture2D(Texture0, startPos) - vec4(0.5), vec4(0.0));
				startPos.x += stepSize;
			}
		}
		else
		{
			// vertical blur
			vec2 startPos = 0.5 * objectPosition.xy + 0.5 - vec2(0.5*correctionSize, -4.5*stepSize);
			gl_FragColor = vec4(0.0);
			for (int i = 0; i < 9; i++)
			{
				gl_FragColor += 0.1 * max(texture2D(Texture0, startPos), vec4(0.0));
				startPos.y += stepSize;
			}
		}
	}
}