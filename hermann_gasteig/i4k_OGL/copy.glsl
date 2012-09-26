uniform sampler2D Texture0; 
varying vec3 objectPosition; 
varying mat4 parameters; 
 
vec2 randomIteration(vec2 seed)
{
   vec2 adder = vec2(0.735, 0.369);
   vec2 mult = vec2(94.34, 72.15);
   return fract((seed.yx + adder) * mult);
}

void main(void) 
{   
	float fTime0_X = parameters[0][0];
   
	// mode 0: Normal copy mode
	// mode 0.2: horizontal blur (including color reduction)
	// mode 0.4: vertical blur
	float mode = parameters[0][1]; 
	vec2 stepSize = parameters[0].zw;
   
	if (mode < 0.1)
	{
		// Normal print to front
		float scanLineAmount = parameters[1][0];

		vec3 noisePos = objectPosition + fTime0_X; 
		vec2 noiseVal;
		noiseVal.x = abs(fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453)); 
		noiseVal.y = abs(fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453));
		gl_FragColor = vec4(0.0);
		for (int i = 0; i < 5; i++)
		{
			noiseVal = randomIteration(noiseVal);
			vec2 step = 2.0 * noiseVal - vec2(1.0);
			gl_FragColor += 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + stepSize*noiseVal.xy);
		}

		gl_FragColor.xyz *= (0.5 * sin(objectPosition.y * 400.0) + 0.5) * scanLineAmount + (1.0 - scanLineAmount);
	}
	else
	{
		// how far I go left/right in one step (I also need to shift left/right for additional blur?)
		float reduction = parameters[1][0];
		float gain = parameters[1][1];

		// horizontal blur (including color reduction)
		vec2 startPos = 0.5 * objectPosition.xy + 0.5 - 19.5 * stepSize;
		gl_FragColor = vec4(0.0);
		for (int i = 0; i < 39; i++)
		{
			float localGain = 19.5 - abs(float(i)-19.5);
			gl_FragColor += localGain * gain * max(pow(texture2D(Texture0, startPos), vec4(reduction)), vec4(0.0));
			startPos += stepSize;
		}
	}
}