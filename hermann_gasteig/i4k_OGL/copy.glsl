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
   
	vec2 stepSize = parameters[0].zw;
   
	// Normal print to front
	float scanLineAmount = parameters[1][0];
	// 0.0: single take, 1.0: multiple take.
	float mode = parameters[0][1];

	if (mode > 0.5)
	{
		gl_FragColor = 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.5 * stepSize);
		gl_FragColor += 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.5 * vec2(stepSize.x, -stepSize.y));
		gl_FragColor += 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.5 * vec2(-stepSize.x, stepSize.y));
		gl_FragColor += 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 - 0.5 * stepSize);
		gl_FragColor += 0.2 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5);
	}
	else
	{
		gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5);
	}

	gl_FragColor.xyz *= (0.5 * sin(objectPosition.y * 500.0) + 0.5) * scanLineAmount + (1.0 - scanLineAmount);
}