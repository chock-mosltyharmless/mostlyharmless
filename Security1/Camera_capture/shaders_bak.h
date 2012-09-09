#pragma once

const GLchar *fragmentSobel="\
uniform sampler2D Texture0;													\n\
uniform sampler2D Texture1;													\n\
uniform sampler2D Texture2;													\n\
varying mat4 parameters;													\n\
varying vec2 position;														\n\
varying vec4 color;															\n\
																			\n\
/* Given a position, this function generates a 3D co-ordinates based,		\n\
 * reconstructible static noise. */											\n\
float noise(vec3 position)													\n\
{																			\n\
	position.x += position.y * 57. + position.z * 21.;						\n\
	return sin(cos(position.x) * position.x);								\n\
																			\n\
	/* The following is an alternative for the previous line:				\n\
	 * return fract(position.x * position.x * .0013) * 2. - 1.; */			\n\
}																			\n\
																			\n\
/* Given a position, this function generates a 3D co-ordinates based,		\n\
 * reconstructible linearly interpolated smooth noise.						\n\
 *																			\n\
 * This function uses the noise() function above for its underlying			\n\
 * noise texture. */														\n\
float smooth_noise(vec3 position)											\n\
{																			\n\
	vec3 integer = floor(position);											\n\
	vec3 fractional = position - integer;									\n\
																			\n\
	return mix(mix(mix(noise(integer),										\n\
			   noise(integer + vec3(1, 0, 0)),								\n\
			   fractional.x),												\n\
		       mix(noise(integer + vec3(0, 1, 0)),							\n\
			   noise(integer + vec3(1, 1, 0)),								\n\
			   fractional.x),												\n\
		       fractional.y),												\n\
		   mix(mix(noise(integer + vec3(0, 0, 1)),							\n\
			   noise(integer + vec3(1, 0, 1)),								\n\
			   fractional.x),												\n\
		       mix(noise(integer + vec3(0, 1, 1)),							\n\
			   noise(integer + 1.), fractional.x),							\n\
		       fractional.y),												\n\
		   fractional.z) * .5 + .5;											\n\
}																			\n\
																			\n\
/* Given a position, this function constructs the oh-so-famous Perlin		\n\
 * noise. */																\n\
float perlin(vec3 position)													\n\
{																			\n\
	return smooth_noise(position * .06125) * .5 +							\n\
	       smooth_noise(position * .125) * .25 +							\n\
	       smooth_noise(position * .25) * .125;								\n\
}																			\n\
																			\n\
float getXSobel()															\n\
{																			\n\
   float distance = 1.0 / 1024.0;											\n\
   vec4 inpt;																\n\
   //inpt = texture2D(Texture0, gl_TexCoord[0].xy + vec2(distance, distance));\n\
   inpt = texture2D(Texture0, gl_TexCoord[0].xy + vec2(distance, 0.));		\n\
   //inpt += texture2D(Texture0, gl_TexCoord[0].xy + vec2(distance, -distance));\n\
   //inpt -= texture2D(Texture0, gl_TexCoord[0].xy - vec2(distance, distance));\n\
   inpt -= texture2D(Texture0, gl_TexCoord[0].xy - vec2(distance, 0.));		\n\
   //inpt -= texture2D(Texture0, gl_TexCoord[0].xy - vec2(distance, -distance));\n\
   inpt = abs(inpt);   													    \n\
   vec4 inpt2;\n\
   inpt2 = texture2D(Texture0, gl_TexCoord[0].xy + vec2(0., distance));		\n\
   inpt2 -= texture2D(Texture0, gl_TexCoord[0].xy - vec2(0., distance));	\n\
   inpt += abs(inpt2);\n\
   return 4. * max(inpt.r, max(inpt.g, inpt.b));							\n\
}																			\n\
																			\n\
vec3 RGB2YUV(vec3 color)\n\
{\n\
	vec3 result;\n\
	result.b =  color.r * 0.299 + color.g * 0.587 + color.b * 0.114;\n\
	result.g = - color.r * 0.147 - color.g * 0.289 + color.b * 0.436;\n\
	result.r =   color.r * 0.615 - color.g * 0.515 - color.b * 0.100;\n\
	//result.r = atan(u, v);\n\
	//result.g = length(vec2(u,v));\n\
	return result;\n\
}\n\
\n\
vec3 getBGColor()\n\
{\n\
	float dx = 1.0 / (512.0+256.0);\n\
	float dy = 1.0 / (256.0+128.0);\n\
	float a = 1.0 / 9.0;\n\
	\n\
	vec4 inpt = vec4(0.0);\n\
	for (float y = -2.0 * dy; y < 2.5 * dy; y += dy)\n\
	{\n\
		for (float x = -2.0 * dx; x < 2.5 * dx; x += dx)\n\
		{\n\
			inpt += texture2D(Texture1, gl_TexCoord[1].xy + vec2(x, y));\n\
		}\n\
	}\n\
	return inpt.rgb / (25.0);\n\
}\n\
\n\
void main(void)\n\
{\n\
   // constants\n\
   \n\
   // standard readout: \n\
   vec4 inpt = texture2D(Texture0, gl_TexCoord[0].xy);\n\
   vec3 fgColor = inpt.rgb;\n\
   float alpha = sqrt(inpt.a);\n\
   float fgBW = dot(vec3(0.3, 0.59, 0.11), fgColor.rgb);\n\
   vec3 bgColor = vec3(0.5);\n\
   if (alpha < 0.99)\n\
   {\n\
	   bgColor = getBGColor();\n\
   }\n\
   float bgBW = dot(vec3(0.3, 0.59, 0.11), bgColor.rgb);\n\
   float sobelX = getXSobel();\n\
   //float visibility = isEqual(fgColor, 0.1, 0.15);\n\
   \n\
   float sobelDistance = parameters[0][0] - position.x;\n\
   if (sobelDistance < 0.0) sobelDistance = 1.0;\n\
   sobelDistance = clamp(sobelDistance, 0.0, 1.0);\n\
   sobelDistance = sqrt(sobelDistance);\n\
   gl_FragColor = vec4(sobelDistance);\n\
   \n\
   if (parameters[0][1] < 0.5) // no fixed function\n\
   {\n\
      float mouseDistance = length(vec2(3.0, 1.0) * (parameters[3].xy - position));\n\
	  float scanIntensity = -2.0 + sqrt(parameters[0][3]) * 3.0 * clamp(1.0 - 10.0 * mouseDistance, 0.0, 1.0);\n\
	  scanIntensity += perlin(vec3(position*500.0, 0.5)) * (2.0 * smoothstep(0.0, 0.3, sobelX) + 1.5);\n\
	  scanIntensity = smoothstep(0.6, 0.8, scanIntensity);\n\
	  vec3 scanColor = mix(vec3(0.9, 0.8, 0.0), vec3(0.75, 0.0, 0.0), scanIntensity);\n\
	  \n\
	  // Calculate the color of the texture in BW or color\n\
	  vec3 fgData = vec3(fgBW);\n\
	  // Scanline hack for fgData\n\
	  if (parameters[1][1] > 0.5) {\n\
	    fgData = fgData * vec3(1.0, 1.075, 1.15);\n\
		if (mod(gl_FragCoord.y, 4.0) < 2.0) { fgData = fgData * 0.9; }\n\
		else { fgData = fgData * 1.1; }\n\
	  }\n\
	  \n\
	  if (parameters[3][3] > 0.5) fgData = fgColor.rgb;\n\
	  \n\
	  if (parameters[1][0] < 0.5) // do sobel\n\
	  {\n\
	     //gl_FragColor = vec4(mix(clamp(vec3(1.-sobelX+scanIntensity), 0.0, 1.0) * scanColor, fgData, sobelDistance), 1.0);\n\
	     //gl_FragColor = mix(gl_FragColor, vec4(fgData, 1.0), 1.0);\n\
		 //gl_FragColor = vec4(fgData, 1.0);\n\
		vec3 hsl = RGB2YUV(fgColor.rgb);\n\
		float hyper = smoothstep(0.2, 0.75, sobelX);\n\
		hyper += smoothstep(0.0, 0.3, length(hsl.rg));\n\
		gl_FragColor = vec4(mix(vec3(hyper) * scanColor, fgData, sobelDistance), 1.0);\n\
	  } else { // do hyperscan\n\
		vec3 hsl = RGB2YUV(fgColor.rgb);\n\
		float hyper = smoothstep(0.2, 0.75, sobelX);\n\
		hyper += smoothstep(0.0, 0.3, length(hsl.rg));\n\
		gl_FragColor = vec4(mix(vec3(hyper) * scanColor, fgData, sobelDistance), 1.0);\n\
		// check for bounding box\n\
		if (position.x < parameters[2][0] || position.y < parameters[2][1] || position.x > parameters[2][2] || position.y > parameters[2][3])\n\
		{ gl_FragColor = vec4(fgData, 1.0); }\n\
	  }\n\
	  //gl_FragColor.a = alpha;\n\
	  gl_FragColor.rgb = mix(vec3(bgBW), gl_FragColor.rgb, alpha);\n\
   } else {\n\
      gl_FragColor = color;\n\
   }\n\
}";

//   if (isEqual(fgColor, 0.1))\n\
//   {\n\
//      gl_FragColor = fgColor;\n\
//   } else {\n\
//      gl_FragColor = vec4(vec3(sobelX+sobelY), 1.);\n\
//   }\n\


const GLchar *vertexMain="\
varying mat4 parameters;\n\
varying vec2 position;\n\
varying vec4 color;\n\
\n\
void main(void)\n\
{\n\
	parameters = gl_ModelViewMatrix;\n\
	position = gl_Vertex.xy;\n\
	\n\
	gl_Position = gl_ProjectionMatrix*gl_Vertex;\n\
	gl_TexCoord[0] = gl_MultiTexCoord0;\n\
	gl_TexCoord[1] = gl_MultiTexCoord1;\n\
	gl_TexCoord[2] = gl_MultiTexCoord1;\n\
	color = gl_Color;\n\
}";

