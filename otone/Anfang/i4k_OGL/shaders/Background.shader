varying mat4 parameters;
uniform sampler2D Texture0;
varying vec3 objectPosition;
varying vec4 color;

void main(void)
{
	float time = parameters[0][0];

    vec4 swirl = texture2D(Texture0, objectPosition.xy * 0.11);
    swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, objectPosition.xy*0.83 + swirl.rg);
	swirl = 0.5 * swirl + 0.5 * texture2D(Texture0, objectPosition.xy*0.14 + 0.2 * swirl.rg + vec2(time * 0.031, 0.0));
	swirl = 0.875 * swirl + 0.125 * texture2D(Texture0, swirl.rg + objectPosition.xy*1.2 + vec2(0.0, time * 0.043));

	vec2 swirlypos = 0.5 * objectPosition.xy + 0.4 * swirl.rg + vec2((time-130.0) * 0.021, 0.0);

	// swirled fog
	vec4 fog = texture2D(Texture0, swirlypos.xy * 0.27);
	fog += 0.5 * texture2D(Texture0, swirlypos.xy * 0.53);
	fog += 0.25 * texture2D(Texture0, swirlypos.xy * 0.98);
	fog += 0.125 * texture2D(Texture0, swirlypos.xy * 2.03);
	fog += 0.2 * texture2D(Texture0, swirlypos.xy * 3.13);
	fog += 0.1 * texture2D(Texture0, swirlypos.xy * 5.02);
	fog = 0.5 * fog;

	// unswirled fog
	fog += 0.03 * texture2D(Texture0, objectPosition.xy * 4.11);
	fog += 0.02 * texture2D(Texture0, objectPosition.xy * 8.03);
	fog += 0.01 * texture2D(Texture0, objectPosition.xy * 16.13);

	// black lines
	//float blackness = smoothstep(1.2, 1.5, length(fog));
	float blackness = smoothstep(0.6, 1.2, fog.r * (1.0 + parameters[0][1]) - 0.5 * parameters[0][1]);

	// red fog
	float redfog = smoothstep(0.6, 1.2, fog.g + parameters[1][2] * fog.b * 0.75);

	//blackness *= parameters[0][1];

	// avoid fog
	redfog *= parameters[0][3];
	blackness *= parameters[0][3];

	gl_FragColor = vec4(vec3(0.0, 0.0, 0.1) + redfog * vec3(1.4, 0.6, 0.5) + parameters[1][2] * vec3(-0.3, 0.4, -0.3) * fog.b*fog.a, (1.0 - blackness) * (1.0 - redfog));
	// saturate on parameters[1][2]
	//gl_FragColor = color;
}
