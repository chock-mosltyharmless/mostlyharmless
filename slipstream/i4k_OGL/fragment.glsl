#version 120
uniform sampler3D Texture0;
varying in vec3 objectPosition;
varying in mat4 parameters;

vec3 noise(vec3 pos, int iterations, float reduction)
{
    pos *= 2.; /*adjust texture size*/
    float intensity = 1.;
    float size = 1.;
    vec3 result = vec3(0.);

    for (int k = 0; k < iterations; k++)
    {
		vec3 pos2 = floor(pos*size*16.) + smoothstep(0.,1., (pos*size*16.) - floor(pos*size*16.)) - 0.5;
        vec4 inp = texture3D(Texture0, pos2/16.);
        result += inp.xyz * intensity;
        intensity = intensity * reduction;
        size = size * 1.93;
    }
    
    return result;
}

vec3 col(vec3 pos, float noiseData)
{
	float fTime0_X = parameters[0][0];
	vec3 innercolor = vec3(0.4, 0.1, 0.0);
	vec3 outercolor = vec3(0.1, 0.35, 0.65);
	float height = (pos.y + noiseData);
	float colorpart = smoothstep(-0.5, 0.5, -length(pos) + noiseData*12.*(parameters[2][1]+1.) + 2.);
	float border = smoothstep(-0.8, 0.8, abs(colorpart - 0.5));
	return mix(innercolor, outercolor, colorpart) * border + clamp(height - 2.*parameters[1][3] - 1.,0.0,100.0);
}

vec2 rotate(vec2 pos, float angle)
{
	return pos * mat2(cos(angle),-sin(angle),sin(angle),cos(angle));
}

void main(void)
{  
    float fTime0_X = parameters[0][0];
    vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));
    vec3 camPos = vec3(0.0, 0.0, -8. + 8. * parameters[0][1]);
    
    // rotate camera around y axis
    float alpha;
    alpha = parameters[0][3]*9.42;
    camPos.xz = rotate(camPos.xz, alpha);
    rayDir.xz = rotate(rayDir.xz, alpha);
    alpha = parameters[0][2]*9.42;
    camPos.yz = rotate(camPos.yz, alpha);
    rayDir.yz = rotate(rayDir.yz, alpha);
    
    vec3 rayPos = camPos;
    float sceneSize = 16.0;
    vec3 totalColor = vec3(0.,0.,0.);
    float stepSize;
    float totalDensity = 0.0;
    vec3 totalColorAdder = (vec3(0.016, 0.012, 0.009)) * (parameters[1][0]);
    
    while(length(rayPos)<sceneSize && totalDensity < 0.95)
    {
        // base head
        vec3 tmpPos = rayPos;
        float base1 = abs(tmpPos.y);
	  float base2 = abs(length(rayPos.xz) - 2.0 + 2.0 * parameters[1][2]) * (0.5 + parameters[2][0]);
	  float socket = length(rayPos + vec3(0., 9., 0.)) - 8.2 + 20.*parameters[2][1];  
	  float base = (max(base1 - 1.1 - parameters[1][3]*20., base2 - 2.0*parameters[1][2] + 1.2));
	  float implicitVal = min(socket, base);
	  //float implicitVal = base;

        vec3 noiseAdder = noise(rayPos * 0.03  - vec3(0.0, fTime0_X*0.02, 0.0), 2, 0.8) * 0.1 * parameters[1][1];
        float noiseVal = noise(noiseAdder*0.3 + rayPos*0.04*vec3(1.0,0.05+0.95*parameters[1][1],1.0) - vec3(0.0, fTime0_X*0.03, 0.0), 7, 0.7).r * 0.6;
        implicitVal -= noiseVal;
        
	  totalColor += totalColorAdder;
        totalDensity += 0.005;
        if (implicitVal < 0.0)
        {
	      float localDensity = 1. - exp(implicitVal);
		 //float localDensity = 0.1;
		 localDensity = (1.-totalDensity) * localDensity;
		 totalColor += localDensity * col(rayPos, noiseVal);
            totalDensity += localDensity ;
        }
        
	  stepSize = (implicitVal) * 0.5;
        stepSize = max(0.03, stepSize);
        rayPos += rayDir * stepSize;
    }
    
    float grad = normalize(rayPos).y;
    totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));
    
    gl_FragColor = vec4(sqrt(smoothstep(0.3, 1.4, totalColor)), 1.0);

}
