uniform sampler2D Texture0;
varying vec3 objectPosition;
varying mat4 parameters;

vec4 noise(vec4 pos, float lod, sampler2D sampTex)
{
   mat3 curOrientation = mat3(1.0);
   vec4 result = vec4(0.0);
   
   vec2 texPos1 = pos.xy;
   vec2 texPos2 = pos.zw;
   
   vec4 tex1 = (texture2DLod(sampTex, texPos1, lod) - 0.5);
   vec4 tex2 = (texture2DLod(sampTex, texPos2, lod) - 0.5);   
   result += (tex1+tex2)*1.5;
   
   return result * 0.5;
}

float getImplicit(vec3 rayPos, float fTime0_X)
{
   float sphere1 = length(rayPos + vec3(-0.7, parameters[0][2], 0.0));
   float sphere2 = rayPos.y - 2.0;
   vec4 noiseAdd = noise(0.02 * rayPos.xzxz + 0.01 * rayPos.yxzy + vec4(10.0), 0.0, Texture0);
   float noise1 = noise(0.5 * noiseAdd + 0.5 * noiseAdd.yzxw, 0.0, Texture0).r * 0.2;

   // Verzerrer vor dem rauskommen
   vec3 modRayPos = rayPos - vec3(0.0, 2.0 * fTime0_X, 0.0);
   float noise2 = noise(0.03 * modRayPos.yxzx + 0.02 * modRayPos.xzyz, 0.0, Texture0).r * parameters[0][3];

   float resultNormal = sqrt(1.0 / (1.0 / (sphere1*sphere1+0.1) +  1.0 / (sphere2*sphere2+0.1))) - 1.3 + noise1 + noise2;
   return resultNormal;
}

vec3 getNormal(vec3 rayPos, float implicitVal, float fTime0_X)
{
   // This e may be bad? I may have problems with noise?
   float normalEpsilon = 0.11;         // Should I get this from a parameter?
   vec2 e = vec2(normalEpsilon, 0.0);
   return normalize(vec3(getImplicit(rayPos + e.xyy, fTime0_X),
                         getImplicit(rayPos + e.yxy, fTime0_X),
                         getImplicit(rayPos + e.yyx, fTime0_X)) - implicitVal);
}

vec3 getLightAmount(vec3 rayPos, float coneSize, float fTime0_X)
{
   // Light amount is distance squared? or exp?
   //vec3 lightPos = vec3(0.7, 1.9*sin(fTime0_X * 0.26), 0.0);
   vec3 lightPos = vec3(0.7, -1.0, 0.0);
   // I need to make this a variable!
   vec3 color = 2.0 * vec3(1.0, 0.8, 0.7) / (pow(length(rayPos-lightPos) / (coneSize+0.1) + 1.0, 2.5));
   return parameters[1][0] * color;
}

// returns lightstep distance in x, lightamount in y, lightcolor in z
vec3 getLightstream(vec3 pos, float coneSize, float fTime0_X)
{
   float radius = 15.0;
   vec3 result;
   
   // TODO: whatever...
   vec4 noiseData = noise(vec4(0.0, 0.0, 0.0, fTime0_X)*0.03 + pos.xyxz * 0.02 + 0.017 * pos.zxyz, 0.0, Texture0);
   //float noiseVal = abs(noiseData.r + noiseData.g + 0.4) + 0.4 * max(0.0, sin(fTime0_X * 0.2) + 0.1) + 0.0;
   float noiseVal = abs(noiseData.r + noiseData.g + 0.3) + 1.0 - parameters[0][1];
   result.x = noiseVal*1.5 + // depends on scaling of noiseVal
              max(0.0, length(pos) - radius); // nothing behind 5.0;   
   result.x = max(result.x, 0.1);
   result.y = pow(max(0.0, 1.0 - 2.0 * noiseVal - length(pos)/radius), 0.7);
   //result.y = max(0.0, 1.0 - 2.0 * noiseVal - length(pos)/radius);
   result.z = noiseVal * 3.0;
   
   noiseData = noise(noiseData * 4.8 + pos.xyxz * 2.5 + 2.7 * pos.zxzy + 2.9 * pos.yzzx + vec4(0.0, fTime0_X, 0.0, 0.0), coneSize*20.0 + 0.0, Texture0);
   noiseVal = smoothstep(0.45 / max(1.0, 8.0*coneSize), 0.45 / max(1.0, 8.0*coneSize) + 0.4, length(noiseData) + 0.05);
   result.y *= (0.0 + noiseVal) * 8.0 / max(1.0, 4.0*coneSize);
   result.z = clamp(result.z - noiseVal, 0.0, 1.0);
   
   return result;
}

void main(void)
{  
   float fTime0_X = parameters[0][0];
   //float refractionCoefficient = 0.9; // Am I sure about that?
   vec2 startCone = vec2(8.9, 4.7);          // Inlined or from parameter?
 
   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));
   vec3 camPos = vec3(0.0, 0.0, -4.0 + 3.0 * parameters[1][1]);
   
   // rotate camera around y axis
   #if 1
   float alpha = parameters[3][2];
   camPos.yz = vec2(cos(alpha)*camPos.y - sin(alpha)*camPos.z,
                    sin(alpha)*camPos.y + cos(alpha)*camPos.z);
   rayDir.yz = vec2(cos(alpha)*rayDir.y - sin(alpha)*rayDir.z,
                    sin(alpha)*rayDir.y + cos(alpha)*rayDir.z);
   alpha = parameters[3][3];
   camPos.xz = vec2(cos(alpha)*camPos.x - sin(alpha)*camPos.z,
                    sin(alpha)*camPos.x + cos(alpha)*camPos.z);
   rayDir.xz = vec2(cos(alpha)*rayDir.x - sin(alpha)*rayDir.z,
                    sin(alpha)*rayDir.x + cos(alpha)*rayDir.z);
   #endif                    
   
   vec3 rayPos = camPos;
   float sceneSize = 20.0;
   vec3 totalColor = vec3(0.);
   float stepSize = 0.0;
   float totalDensity = 0.0;
   
   // cone properties
   float coneSize = startCone.x * 0.001;
   float coneSizeIncrease = startCone.y * 0.001;
   
   // First ray hit calculation
   for (int step = 0; length(rayPos) < sceneSize && totalDensity < 0.99 && step < 100; step++)
   {
      float implicitVal;
      implicitVal = getImplicit(rayPos, fTime0_X);

#if 1
      // lightstream data
      vec3 lightstream = getLightstream(rayPos, coneSize, fTime0_X);
      // update lightstream color;
      float localDensity = clamp(lightstream.y * stepSize, 0.0, 1.0);
      vec3 lightstreamColor = mix(vec3(1.7, 1.4, 1.1), vec3(1.7, 0.8, 0.4), lightstream.z);
      totalColor += (1.-totalDensity) * lightstreamColor * localDensity;
      totalDensity += (1.-totalDensity) * localDensity;
#else
	  vec3 lightstream = vec3(1000.0, 0.0, 0.0);
#endif
            
      // This is done on a hit. Always 100% coverage!
      if (implicitVal < 0.5 * coneSize)
      {
         vec3 implicitNormal = getNormal(rayPos, implicitVal, fTime0_X);
         if (dot(implicitNormal, rayDir) > 0.0)
         {
            implicitNormal -= rayDir * dot(implicitNormal, rayDir);
         }
         
         // Calculate refracted ray
         vec3 refractRayDir = rayDir;
		 vec3 refractRayPos = rayPos - implicitNormal * 0.05;
         refractRayDir = refract(rayDir, implicitNormal, 0.7);
         float refractConeSize = coneSize*5.0;
         float refractConeSizeIncrease = coneSizeIncrease;
         
         // initialize data for refracted light calculation
         vec3 subtractiveRayColor = vec3(1.0); // multiplicative term
         vec3 additiveLightAmount = vec3(0.0); // takes subtractiveRayColor into consideration.

         // move forward along the refracted ray
         for (int refractStep = 0;
			  length(refractRayPos) < sceneSize && totalDensity < 0.99 && refractStep < 10;
			  refractStep++)
         {
             stepSize = refractConeSize * 0.5;
			              
             // adjust lighting data
             vec3 tightness = 10.0*vec3(0.30, 0.22, 0.15);
             vec4 noiseData = noise(0.05 * refractRayPos.xyzx + 0.15 * refractRayPos.zxyz, refractConeSize*1.0, Texture0);
             float noiseVal = smoothstep(0.0, 0.4, 0.4 - length(noiseData.rg));
             tightness *= 0.3*pow(noiseVal*2.0, 1.5) + 0.15;
             vec3 localDensity = exp(-tightness * stepSize); // standard fog formula             
             subtractiveRayColor *= localDensity; // Only gray yet
             //additiveLightAmount += getLightAmount(rayPos, coneSize) * subtractiveRayColor * stepSize;
             additiveLightAmount += getLightAmount(refractRayPos, refractConeSize, fTime0_X) * subtractiveRayColor;
             //additiveLightAmount += vec3(1.0) / (refractRayPos + vec3(1.0)) * subtractiveRayColor;

             // cone update
             refractRayPos += refractRayDir * stepSize;
             refractConeSize += refractConeSizeIncrease * stepSize;
             float refractImplicitVal = getImplicit(refractRayPos, fTime0_X);
             if (refractImplicitVal < 0.0)
             {
				// Drastically increase the stepsize inside the refracted stuff
                refractConeSizeIncrease += stepSize * 1.0 + 0.5; // what?
             }
         }
     
         // update color, TODO localDensity modification?
         float localDensity = clamp(-dot(rayDir, implicitNormal), 0.0, 1.0);
         // strength of reflection
         localDensity = pow(localDensity, 0.65);
         //localDensity = 1.0;
         //vec3 addColor = additiveLightAmount + subtractiveRayColor;
         // This was wrong and created the light at the border of the scene?
		 vec3 addColor = additiveLightAmount;
		 totalColor += (1.-totalDensity) * addColor * localDensity;
         totalDensity += (1.-totalDensity) * localDensity;
                  
#if 1
         // do reflection:
         // TODO: I need to move the ray along the normal in
         // order to avoid self-reflection. Do this properly!
		 step += 20; // decrease the amount of time spent here
         rayDir = reflect(rayDir, implicitNormal);
         rayPos += implicitNormal * 0.1;
         
         // This is a hack so that color reflection is not so bright...
         localDensity = parameters[3][1];
         coneSizeIncrease *= 10.0;
         //totalDensity += (1.-totalDensity) * localDensity;
#else
		 rayPos = vec3(10000.0,0.0,0.0);
#endif
         
         localDensity = 1.0 - exp(-localDensity * stepSize); // standard fog formula
         //totalColor = totalColor + (1.-totalDensity) * vec3(1.0,0.7,0.3) * localDensity;
         totalDensity = totalDensity + (1.-totalDensity) * localDensity;
      }
      
      stepSize = min(lightstream.x, implicitVal) * 0.7;
      //stepSize = implicitVal * 0.9;
      // Here I need widening min stepsizing!
      stepSize = max(0.5 * coneSize, stepSize);
      
      // cone update
      rayPos += rayDir * stepSize;
      coneSize += coneSizeIncrease * stepSize;
   }
     
   gl_FragColor = vec4(totalColor-vec3(0.0), 1.0);
}