uniform sampler2D Texture0;
varying vec3 objectPosition;
varying mat4 parameters;

vec4 randomIteration(vec4 seed)
{
   vec4 adder = vec4(0.735, 0.369, 0.438, 0.921);
   vec4 mult = vec4(94.34, 72.15, 59.372, 49.56);
   return fract((seed.zxwy + adder) * mult);
}

/* period 1, minimum at 0, output 0..1, probably does not work for neagtive values */
vec3 sawtooth(vec3 x)
{
    return 2.0 * min(fract(x), 1.0 - fract(x));
}

float brightness(vec3 color)
{
    return dot(color, vec3(0.3, 0.55, 0.15));
}

/* brightness is the only thing that may have values > 1.0 */
vec3 HSB2RGB(vec3 hsb)
{
    vec3 rgb;
    vec3 gray;

    /* Calculation for hue (vectorize!) */
    rgb = max(vec3(1.0) - 3.0/2.0 * sawtooth(vec3(hsb.r) + vec3(0.0, 1.0/3.0, 2.0/3.0)), vec3(0.0));
    
    /* normalize brightness */
    gray = vec3(1.0);
    gray /= brightness(gray);
    rgb /= brightness(rgb);
    
    /* saturation */
    rgb = mix(gray, rgb, hsb.g);
    
    /* brightness */
    rgb *= hsb.b;
    
    return rgb;
}

void main(void)
{
    float fTime0_X = parameters[0][0];
	float bauchigkeit = parameters[0][1];
	float lineStrength = parameters[0][2];
	float colorVariation = parameters[0][3];
	vec2 size = parameters[1].xy;
	vec2 spread = parameters[1].zw;
	vec3 mainColorHSB = parameters[2].xyz;
	float highlightAmount = parameters[2][3];

	/* Color changes over time */
	mainColorHSB.r = fract(mainColorHSB.r + fTime0_X * 0.05);

    vec2 position = objectPosition.xy * 3.;
    vec4 n;
    const int numCenters = 8;
    const int numOvertones = 3;
    
    /* distance of best and second best center */
    float nearestDist = 1.0e20;
    float secondDist = 1.0e20;
    vec2 bestCenter = vec2(0.0); /* Position of the best center */
    vec2 bestMover = vec2(0.0);  /* move based on center */

    /* Calculate distances to randomly movind centers */
    vec4 seed = vec4(0.3, 0.2, 0.1, 0.9);
    vec3 baseColor = vec3(0.0);
    for (int i = 0; i < numCenters; i++)
    {
        /* Calculate position of the center */
        vec2 center = vec2(0.0);
        for (int j = 0; j < numOvertones; j++)
        {
            seed  = randomIteration(seed);
            center += vec2(size.x * sin(fTime0_X * seed.x + seed.z) + 2.0 * (seed.y - 0.5) * spread.x,
                           size.y * sin(fTime0_X * seed.z + seed.w) + 2.0 * (seed.w - 0.5) * spread.y);
        }
        seed = randomIteration(seed);
        
        /* Get the distance to the center */
        float curDist = length(position - center);
        if (curDist < secondDist)
        {
            if (curDist < nearestDist)
            {
                secondDist = nearestDist;
                nearestDist = curDist;
                bestCenter = center;
                bestMover = seed.yz;
                vec3 internalHSB = mainColorHSB + colorVariation *
                    (seed.rgb - 0.5) * vec3(1.0, 0.0, 0.0);
                internalHSB.r = fract(internalHSB.r + 0.5); /* + 0.5 so that I get inverted colors with the border) */
                internalHSB.g = clamp(internalHSB.g, 0.0, 1.0);
                baseColor = HSB2RGB(internalHSB);
            }
            else
            {
                secondDist = curDist;
            }
        }
    }
    
    /* Calculate the relative distance to the nearest point */
    /* What we get here is a float between 0.0 and 1.0 */
    float relDist = 2.0 * nearestDist / (nearestDist + secondDist + 0.0);
    //relDist = mix(smoothstep(0., 1., relDist), relDist, 0.5);
    
    /* non-linear things */
    //relDist = smoothstep(0.1, 1.1, relDist);
    relDist = pow(relDist, bauchigkeit);
    
    /* Here I need the base color instead */
    vec3 bgColor = mix(HSB2RGB(mainColorHSB), vec3(0.0), lineStrength);
        
    gl_FragColor = vec4(mix(baseColor, bgColor, relDist), 1.0);
    
    /* Some highlights due to lighting */
    vec2 normal2D = normalize(position - bestCenter);
    float jumpDist = floor(relDist*10.)/8.0;
    vec3 normal3D = vec3(normal2D * sqrt(jumpDist), sqrt(1.0 - jumpDist));
    vec3 lightsource = normalize(vec3(0.4, 0.6, 0.3));
    float lighting = highlightAmount * (pow(max(0., dot(normal3D, lightsource) - 0.2), 3.0));
    gl_FragColor += vec4(vec3(lighting), 0.0);

    /* vignette */
    float vignette = length(position * vec2(0.7, 1.0));
    gl_FragColor /= pow(0.5*vignette, 5.0) + 1.0;    
}