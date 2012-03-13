varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

float getImplicit(vec2 relPos)
{
	vec4 noiseV = texture2D(Texture0, relPos * 0.31) +
				0.5 * texture2D(Texture0, relPos * 0.57) +
				0.25 * texture2D(Texture0, relPos * 0.91);
	float implicitVal = length(noiseV.r - 1.0) -
		0.1 * parameters[0][1] +
		//1.5 * parameters[0][1] * abs(relPos.x);
		0.3 * length(relPos * vec2(1.0, 0.5));
	return implicitVal;
}

vec2 getImplicitNormal(vec2 relPos)
{
	vec2 implicitNormal;
	float implicitVal = getImplicit(relPos);
	implicitNormal.x = getImplicit(relPos + vec2(0.02, 0.0)) - getImplicit(relPos - vec2(0.02, 0.0));
	implicitNormal.y = getImplicit(relPos + vec2(0.0, 0.02)) - getImplicit(relPos - vec2(0.0, 0.02));
	implicitNormal /= (length(implicitNormal) + 0.00001);
	
	implicitNormal = mix(implicitNormal, relPos, smoothstep(0.0, 0.1, implicitVal));
	implicitNormal /= (length(implicitNormal) + 0.00001);

	return implicitNormal;
}

void main(void)
{
	float time = parameters[0][0];

	vec2 relPos = objectPosition.xy - vec2(parameters[2][0], parameters[2][1]);

	// Do the light calculation here
	float implicitVal = getImplicit(relPos);
	//vec2 implicitNormal = getImplicitNormal(relPos);
	//if (implicitVal > 0.0)
	//{
	//	relPos += parameters[0][1] * implicitNormal / pow(2.0, (implicitVal)) * 0.1;
	//}

	relPos += vec2(parameters[2][0], parameters[2][1]);

    vec4 swirl = texture2D(Texture0, relPos * 0.11);
    swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, relPos*0.83 + swirl.rg);
	// apply some shattering
	//swirl = (swirl-0.5) * (sqrt(parameters[0][1]) + 0.5) * 2.0;
	swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, relPos*0.14 + 0.2 * swirl.rg + vec2(time * 0.031, 0.0));
	swirl = 0.875 * swirl + 0.125 * texture2D(Texture0, swirl.rg + relPos*1.2 + vec2(0.0, time * 0.043));	

	vec2 swirlypos = 0.5 * relPos + 0.4 * swirl.rg + vec2((time-130.0) * 0.021, 0.0);

	// swirled fog
	vec4 fog = texture2D(Texture0, swirlypos.xy * 0.27);
	fog += 0.5 * texture2D(Texture0, swirlypos.xy * 0.53);
	fog += 0.25 * texture2D(Texture0, swirlypos.xy * 0.98);
	fog += 0.125 * texture2D(Texture0, swirlypos.xy * 2.03);
	fog += 0.2 * texture2D(Texture0, swirlypos.xy * 3.13);
	fog += 0.1 * texture2D(Texture0, swirlypos.xy * 5.02);
	fog = 0.5 * fog;

	// unswirled fog
	fog += 0.03 * texture2D(Texture0, relPos * 4.11);
	fog += 0.02 * texture2D(Texture0, relPos * 8.03);
	fog += 0.01 * texture2D(Texture0, relPos * 16.13);

	// black lines
	//float blackness = smoothstep(1.2, 1.5, length(fog));
	//float blackness = smoothstep(0.6, 1.2, fog.r * (1.0 + parameters[0][1]) - 0.5 * parameters[0][1]);
	float blackness = smoothstep(0.5, 1.1, fog.r * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	// red fog
	//float redfog = smoothstep(0.6, 1.2, fog.g + parameters[1][2] * fog.b * 0.75);
	float redfog = smoothstep(0.5, 1.1, fog.g * (1.0 - 0.75 * parameters[0][1]) + 0.75 * parameters[0][1]);

	//blackness *= parameters[0][1];

	// avoid fog
	redfog *= parameters[0][3];
	blackness *= parameters[0][3];

	// some add if there is fog
	vec3 blackFogAdder = color.rgb * blackness;
	vec3 redFogAdder = redfog * mix(vec3(1.2, 0.8, 0.2), vec3(1.0, 1.0, 1.0), redfog);
	//vec3 parameter6Adder = parameters[1][2] * vec3(-0.3, 0.4, -0.3) * fog.b*fog.a;
	
	// fog overdrive
	blackFogAdder *= vec4(1.0 + parameters[1][2]);
	redFogAdder *= vec4(1.0 + parameters[1][2]);

	//gl_FragColor = vec4(redFogAdder + blackFogAdder, (1.0 - blackness) * (1.0 - redfog));
	gl_FragColor = vec4(redFogAdder + blackFogAdder, 1.0);
	// saturate on parameters[1][2]
	//gl_FragColor = color;

	// go down on lightning
	gl_FragColor -= parameters[0][1] * vec4(0.0, 0.6, 0.9, 0.0);

	// add lightning
	if (parameters[0][1] > 0.0)
	{
		float lightningAmount = pow(max(0.0, (1.0 - implicitVal * 2.0)), 8.0);
		gl_FragColor += lightningAmount * vec4(1.0, 0.6, 0.3, 0.0);
	}
}
