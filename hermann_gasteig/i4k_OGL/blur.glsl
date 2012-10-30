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
   
	// how far I go left/right in one step (I also need to shift left/right for additional blur?)
	float reduction = parameters[1][0];
	float gain = parameters[1][1];

	// horizontal blur (including color reduction)
	vec2 startPos = 0.5 * objectPosition.xy + 0.5 - 29.5 * stepSize;
	gl_FragColor = vec4(0.0);
	for (int i = 0; i < 59; i++)
	{
		//float localGain = 29.5 - abs(float(i)-29.5);
		float localGain = 10.0;
		gl_FragColor += localGain * gain * max(pow(texture2D(Texture0, startPos), vec4(reduction)), vec4(0.0));
		startPos += stepSize;
	}
}