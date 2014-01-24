varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;

vec2 getFog(vec2 relPos)
{
	float time = parameters[0][0];

    vec4 swirl = texture2D(Texture0, relPos * 0.11);
    swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, relPos*0.63 + swirl.rg);
	// apply some shattering
	swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, relPos*0.14 + 0.2 * swirl.rg + vec2(time * 0.031, 0.0));
	swirl = 0.875 * swirl + 0.125 * texture2D(Texture0, swirl.rg + relPos*1.2 + vec2(0.0, time * 0.043));	
	
	swirl = 0.925 * swirl + 0.075 * texture2D(Texture0, swirl.rg + relPos*2.2 + vec2(time * 0.023, time * 0.033));	

	vec2 swirlypos = 0.5 * relPos + 0.4 * swirl.rg + vec2((time-130.0) * 0.021, 0.0);

	// swirled fog
	vec4 fog = texture2D(Texture0, swirlypos.xy * 0.13);
	fog += 0.5 * texture2D(Texture0, swirlypos.xy * 0.53);
	fog += 0.25 * texture2D(Texture0, swirlypos.xy * 1.06);
	fog += 0.125 * texture2D(Texture0, swirlypos.xy * 2.06);
	fog += 0.2 * texture2D(Texture0, swirlypos.xy * 3.26);
	fog += 0.1 * texture2D(Texture0, swirlypos.xy * 4.99);
	fog = 0.5 * fog;

	fog = 0.55 * fog + 0.6 * smoothstep(0.0, 0.95, swirl);

	// unswirled fog
	fog += 0.03 * texture2D(Texture0, relPos * 4.11);
	fog += 0.02 * texture2D(Texture0, relPos * 8.03);
	fog += 0.01 * texture2D(Texture0, relPos * 16.13);

	// black lines
	float blackness = smoothstep(0.5, 1.1, fog.r * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	// red fog
	float redfog = smoothstep(0.5, 1.1, fog.g * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	// avoid fog
	//redfog *= parameters[0][3];
	//blackness *= parameters[0][3];
	redfog -= 1.0f - parameters[0][3];
	blackness -= 1.0f - parameters[0][3];
	redfog = clamp(redfog, 0.0, 10.0);
	blackness = clamp(blackness, 0.0, 10.0);

	return vec2(blackness, redfog);
}

void main(void)
{
	float time = parameters[0][0];

	// hackadoodle
	vec2 relPos = vec2(parameters[3][0], parameters[3][1]) * objectPosition.x * 10./9. +
				  vec2(-parameters[3][1], parameters[3][0]) * objectPosition.y;

	relPos -= vec2(parameters[2][0], parameters[2][1]);

	vec2 fog = getFog(relPos);
	//vec2 fogUp = getFog(relPos + vec2(0.01, 0.));
	//vec2 fogRight = getFog(relPos + vec2(0., 0.01));

	// lighting
	//float magFog = length(fog);
	//float magFogUp = length(fogUp);
	//float magFogRight = length(fogRight);
	//vec3 normal = normalize(vec3(magFogRight - magFog, magFogUp - magFog, 0.1));
	//float lighting = dot(normal, normalize(vec3(0.3, 0.5, 0.5))) + 1.0;

	float blackness = fog.r;
	float redfog = fog.g;

	// some add if there is fog
	vec3 blackFogAdder = vec3(0.3, 0.8, 1.2) * blackness;
	vec3 redFogAdder = redfog * mix(vec3(1.4, 1.0, 0.2), vec3(1.2, 1.2, 0.7), redfog);
	
	// fog overdrive
	blackFogAdder *= vec3(1.0 + parameters[1][2]);
	redFogAdder *= vec3(1.0 + parameters[1][2]);

	// lighting
	//blackFogAdder *= lighting * 0.8;
	//redFogAdder *= lighting * 0.8;

   float grad = objectPosition.y * 0.5 + 0.5;
   vec3 totalColor = (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));

	gl_FragColor = vec4(redFogAdder + blackFogAdder + totalColor, 1.0);
	//gl_FragColor = swirl;
}
