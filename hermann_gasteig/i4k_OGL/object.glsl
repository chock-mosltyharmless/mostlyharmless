#version 120 
const int numCenters = 8;
const int numOvertones = 3;
varying vec2 centers[numCenters];
varying vec3 objectPosition;
varying mat4 parameters;

vec4 randomIteration(vec4 seed)
{
   vec4 adder = vec4(0.735, 0.369, 0.438, 0.921);
   vec4 mult = vec4(94.34, 72.15, 59.372, 49.56);
   return fract((seed.zxwy + adder) * mult);
}

void main(void)
{
    parameters = gl_ModelViewMatrix;
    objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);
    gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);
	float fTime0_X = parameters[0][0];
	vec2 size = parameters[1].xy;
	vec2 spread = parameters[1].zw;
#if 1
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
		centers[i] = center;
	}
#endif
}