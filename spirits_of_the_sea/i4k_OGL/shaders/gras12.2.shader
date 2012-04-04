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

vec2 getImplicit(vec3 rayPos, float fTime0_X)
{
   float sphere1 = length(rayPos - vec3(2.0 + 3.0 * parameters[2][0], 0.0, 0.0)) - 3.0 * parameters[2][0];
   float sphere2 = 6.0 - length(rayPos);
   vec4 noiseAdd = noise(0.04 * rayPos.xzxz + 0.043 * rayPos.yxzy + parameters[1] + vec4(fTime0_X) * 0.002, 0.0, Texture0);
   //noiseAdd += 0.15 * noise(0.13 * rayPos.xzxz + 0.12 * rayPos.yxzy, 0.0, Texture0);
   //vec4 noiseAdd2 = noise(vec4(fTime0_X) * 0.03 + noiseAdd * smoothstep(-10.0, 20.0, vec4(fTime0_X)) * 0.5 / max(1.0, sphere1 - 0.5), 0.0, Texture0);
   vec4 noiseAdd2 = noise(0.1 + vec4(fTime0_X) * 0.03 + noiseAdd * 0.8 / max(1.0, 2.0 * sphere1 - 0.5), 0.0, Texture0);
   float baseVal = sqrt(1.0 / (1.0 / (sphere1*sphere1+0.1) +  1.0 / (sphere2*sphere2+0.1))) - 0.8 - length(noiseAdd) * 1.0 + parameters[2][0];// + noiseV1;
   
   //float noise2Amount = smoothstep(0.85, 1.0, 1.0 - length(noiseAdd.rg));
   //float noiseV2 = noise(noiseAdd * 1.50, 0.0, Texture0).r * 0.5;
   
   float baseVal2 = (length(noiseAdd2.rb) + 0.001 + sphere1*0.02 + 0.02 * parameters[3][3]);
   baseVal = min(baseVal, baseVal2);

   //return vec2(baseVal, noise2Amount);
   return vec2(baseVal, max(0.0, 1.0 - parameters[3][3] * sphere1 - 2.0 * baseVal2));
}

vec3 getNormal(vec3 rayPos, float implicitVal, float fTime0_X)
{
   // This e may be bad? I may have problems with noise?
   float normalEpsilon = 0.07;
   vec2 e = vec2(normalEpsilon, 0.0);
   return normalize(vec3(getImplicit(rayPos + e.xyy, fTime0_X).r,
                         getImplicit(rayPos + e.yxy, fTime0_X).r,
                         getImplicit(rayPos + e.yyx, fTime0_X).r) - implicitVal);
}

vec3 getLightAmount(vec3 rayPos, float coneSize, float fTime0_X)
{
   // Light amount is distance squared? or exp?
   vec3 lightPos = 0.9 * vec3(5.0, 2.0, -2.0);
   // I need to make this a variable!
   //vec3 color = 0.04 * (sin(fTime0_X*5.0) * 2.0 + 8.0) * vec3(1.0, 0.8, 0.6) / (pow(length(rayPos-lightPos) / (coneSize+0.3) + 1.0, 1.3));
   vec3 color = 0.04 * max(0.0, (sin(fTime0_X*1.0) * 10.0 + 5.0)) * vec3(1.0, 0.8, 0.6) / (pow(length(rayPos-lightPos) / (coneSize+0.3) + 1.0, 1.3));

   lightPos = vec3(2.0, 0.0, 0.0);
   // I need to make this a variable!
   color += 0.8 * vec3(1.0, 0.5, 0.3) / (pow(length(rayPos-lightPos) / (coneSize+0.3) + 1.0, 2.2));
   
   color *= parameters[0][1];

   return color;
}

void main(void)
{  
   float fTime0_X = parameters[0][0];
   float refractionCoefficient = 0.73;
   //vec2 startCone = vec2(8.9, 4.7);
   vec2 startCone = vec2(12.0, 4.2);

   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));
   vec3 camPos = vec3(0.0, 0.0, -4.0 + 0.0 * sin(fTime0_X*0.5));
   
   // rotate camera around y axis
   #if 1
   float alpha = sin(fTime0_X * 0.27) * 0.0 - 0.0;
   camPos.yz = vec2(cos(alpha)*camPos.y - sin(alpha)*camPos.z,
                    sin(alpha)*camPos.y + cos(alpha)*camPos.z);
   rayDir.yz = vec2(cos(alpha)*rayDir.y - sin(alpha)*rayDir.z,
                    sin(alpha)*rayDir.y + cos(alpha)*rayDir.z);
   alpha = 0.0 - fTime0_X * 0.4;
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
   for(int step = 0; length(rayPos) < sceneSize && totalDensity < 0.99 && step < 300; step++)
   {
      vec2 implicitVec;
      implicitVec = getImplicit(rayPos, fTime0_X);
      float implicitVal = implicitVec.r;

	  //totalDensity += 0.005 * implicitVec.g;
	  totalDensity += 0.007;
	  totalColor += implicitVec.g * parameters[0][1] * (1.0 - totalDensity) * smoothstep(0.0, 1.0, 1.0 - implicitVal) * 3.0 * vec3(0.013, 0.01, 0.005);
      
      // This is done on a hit. Always 100% coverage!
      if (implicitVal < 0.5 * coneSize)
      {
         vec3 implicitNormal = getNormal(rayPos, implicitVal, fTime0_X);
         if (dot(implicitNormal, rayDir) > 0.0)
         {
            implicitNormal -= rayDir * dot(implicitNormal, rayDir);
         }
         
         // Calculate refracted ray
         vec3 refractRayDir = rayDir, refractRayPos = rayPos - implicitNormal * 0.2;
         refractRayDir = refract(rayDir, implicitNormal, refractionCoefficient);         
         float refractConeSize = 10.0  * coneSize;
         float refractConeSizeIncrease = coneSizeIncrease;
         
         // initialize data for refracted light calculation
         vec3 subtractiveRayColor = vec3(1.0); // multiplicative term
         vec3 additiveLightAmount = vec3(0.0); // takes subtractiveRayColor into consideration.
         
         // move forward along the refracted ray
         while (length(refractRayPos) < sceneSize && totalDensity < 0.99)
         {
             stepSize = refractConeSize * 0.5;
             vec2 refractImplicitVec = getImplicit(refractRayPos, fTime0_X);
                          
             // adjust lighting data
             //vec3 tightness = 10.0*vec3(0.35, 0.15, 0.15);
			 vec3 tightness = 10.0*vec3(0.25, 0.15, 0.15);
             vec4 noiseData = noise(0.04 * refractRayPos.xyzx + 0.03 * refractRayPos.zxyz, refractConeSize*0.2, Texture0);
             float noiseVal = smoothstep(0.0, 0.4, 0.4 - length(noiseData.rg));
             tightness *= 0.3*pow(noiseVal*2.0, 1.5) + 0.15;
             vec3 localDensity = exp(-tightness * stepSize); // standard fog formula             
             subtractiveRayColor *= localDensity; // Only gray yet
             additiveLightAmount += getLightAmount(refractRayPos, refractConeSize, fTime0_X) * subtractiveRayColor;
             
             // cone update
             refractRayPos += refractRayDir * stepSize;
             refractConeSize += refractConeSizeIncrease * stepSize;
             float refractImplicitVal = refractImplicitVec.r;
             if (refractImplicitVal < 0.0)
             {
                //refractConeSizeIncrease += stepSize * 0.75; // what?
                refractConeSizeIncrease += stepSize * 1.8; // what?
             }
         }
         
         // update color, TODO localDensity modification?
         float localDensity = clamp(-dot(rayDir, implicitNormal), 0.0, 1.0);
         //localDensity = mix(localDensity, 1.0, implicitVec.g);
		 localDensity = mix(localDensity, 1.0, 0.2);
         // strength of reflection
         localDensity = pow(localDensity, 0.65);
         vec3 addColor = additiveLightAmount;
         //addColor = mix(addColor, addColor * vec3(0.3, 0.5, 0.7), implicitVec.g);
		 addColor = mix(addColor, addColor * vec3(0.3, 0.5, 0.7), 0.2);
         totalColor += (1.-totalDensity) * addColor * localDensity;
         totalDensity += (1.-totalDensity) * localDensity;
                  
         // do reflection:
         // TODO: I need to move the ray along the normal in
         // order to avoid self-reflection. Do this properly!
         rayDir = reflect(rayDir, implicitNormal);
         rayPos += implicitNormal * 0.2;
         
         // This is a hack so that color reflection is not so bright...
		 //localDensity = clamp(-0.1 + implicitVec.g, 0.0 , 1.0);
		 localDensity = 0.1;
         coneSizeIncrease *= 10.0;
         totalDensity += (1.-totalDensity) * localDensity;
      }
      
      stepSize = implicitVal * 0.5;
      //stepSize = min(stepSize, 2.0 * pow(0.5 * stepSize, 1.2));
      //stepSize = min(implicitVal * 0.5, pow(implicitVal + 0.05, 2.0));
      //stepSize = implicitVal * 0.9;
      // Here I need widening min stepsizing!
      stepSize = max(0.5 * coneSize, stepSize);
      
      // cone update
      rayPos += rayDir * stepSize;
      coneSize += coneSizeIncrease * stepSize;
   }
   
   gl_FragColor = vec4(totalColor-vec3(0.0), 1.0);
}