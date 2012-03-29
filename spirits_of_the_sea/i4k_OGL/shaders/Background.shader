varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;

void main(void)
{
	float time = parameters[0][0];

	vec2 relPos = objectPosition.xy;

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

	fog = 0.6 * fog + 0.65 * smoothstep(0.0, 0.95, swirl);

	// unswirled fog
	fog += 0.03 * texture2D(Texture0, relPos * 4.11);
	fog += 0.02 * texture2D(Texture0, relPos * 8.03);
	fog += 0.01 * texture2D(Texture0, relPos * 16.13);

	// black lines
	float blackness = smoothstep(0.5, 1.1, fog.r * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	// red fog
	float redfog = smoothstep(0.5, 1.1, fog.g * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	// avoid fog
	redfog *= parameters[0][3];
	blackness *= parameters[0][3];

	// some add if there is fog
	vec3 blackFogAdder = vec3(0.3, 0.8, 1.2) * blackness;
	vec3 redFogAdder = redfog * mix(vec3(1.4, 1.0, 0.2), vec3(1.0, 1.0, 1.0), redfog);
	
	// fog overdrive
	blackFogAdder *= vec3(1.0 + parameters[1][2]);
	redFogAdder *= vec3(1.0 + parameters[1][2]);

	gl_FragColor = vec4(redFogAdder + blackFogAdder, 1.0);
	//gl_FragColor = swirl;
}
