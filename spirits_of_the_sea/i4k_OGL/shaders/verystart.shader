uniform sampler2D Texture0;
varying vec3 objectPosition;
varying mat4 parameters;

vec4 noise(vec4 pos, float lod, sampler2D sampTex)
{
   mat3 curOrientation = mat3(1.0);
   vec4 result = vec4(0.0);
   
   vec2 texPos1 = pos.xy;
   vec2 texPos2 = pos.zw;

   //vec4 tex1 = normalize(texture2D(Texture0, texPos1) - 0.5);
   //vec4 tex2 = normalize(texture2D(Texture0, texPos2) - 0.5);
   vec4 tex1 = texture2DLod(sampTex, texPos1, lod) - 0.5;
   vec4 tex2 = texture2DLod(sampTex, texPos2, lod) - 0.5;   
   //result += tex1.rgba*tex2.rgba + tex1.gbar*tex2.gbar + tex1.barg*tex2.barg + tex1.argb*tex2.argb;
   result += (tex1+tex2);
   
   return result * 0.5;
}

#if 0
vec3 color(vec3 pos, vec3 noiseStuff, vec3 colMult)
{   
   float dist = length(pos);
   float color = smoothstep(0.3, 1.2, dist);
   float brightness = smoothstep(-1.5, 0.0, -color);
   float stuffi = noiseStuff.g * noiseStuff.b;
   
   colMult -= 20.*(stuffi-0.) * (0.4, 0.6, 0.5);
   return 2.*sqrt(0.25-abs(stuffi)) * (mix(vec3(1.0)-colMult, colMult, color) * brightness + 0.2 * (1.0-brightness));
}
#else
vec3 color(vec3 pos, vec3 noiseStuff, vec3 colMult)
{
   return vec3(-0.6, -0.3, 0.2);
}
#endif

void main(void)
{  
   float fTime0_X = parameters[0][0];

   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));
   vec3 camPos = vec3(-3.0, 5.0, -15.0 + 0.0 * sin(fTime0_X*0.5));
   
   // rotate camera around y axis
   #if 1
   float alpha = -0.2;
   camPos.yz = vec2(cos(alpha)*camPos.y - sin(alpha)*camPos.z,
                    sin(alpha)*camPos.y + cos(alpha)*camPos.z);
   rayDir.yz = vec2(cos(alpha)*rayDir.y - sin(alpha)*rayDir.z,
                    sin(alpha)*rayDir.y + cos(alpha)*rayDir.z);
   alpha = fTime0_X * 0.3;
   camPos.xz = vec2(cos(alpha)*camPos.x - sin(alpha)*camPos.z,
                    sin(alpha)*camPos.x + cos(alpha)*camPos.z);
   rayDir.xz = vec2(cos(alpha)*rayDir.x - sin(alpha)*rayDir.z,
                    sin(alpha)*rayDir.x + cos(alpha)*rayDir.z);
   #endif
   
   vec3 rayPos = camPos;
   float sceneSize = 20.0;
   vec3 totalColor = vec3(0.);
   float stepSize;
   float totalDensity = 0.0;

#if 0
   float totalSize = (sin(fTime0_X * 0.1) + 1.0);
   totalSize *= totalSize;
   totalSize *= totalSize; 
   totalSize *= totalSize * 0.4;
#else
	float totalSize;
#endif
   //totalSize = -1.2;
   totalSize = noise(vec4(fTime0_X*0.05), 0., Texture0).r * 1.0 - 2.0;
   //totalSize += sin(fTime0_X*1.0) * 10.0 + 10.0;
   totalSize -= 1.9 * parameters[0][1];
   totalSize = -2.;
      
   for (int run = 0; run < 200 && length(rayPos)<sceneSize && totalDensity < 0.95; run++)
   {
      vec3 tmpPos = rayPos;   
   
      vec4 noiseData = noise(-tmpPos.xyyz*0.2*pow(length(tmpPos)+1.0, -0.8) + vec4(0.0, fTime0_X*0.03, 0.0, 0.), length(tmpPos)*.1, Texture0);
      float firstNoise = noiseData.r;
      firstNoise = clamp(0.4 - abs(firstNoise), 0.0, 0.4);
      firstNoise *= firstNoise * 3.;

      //vec3 noiseData2 = noise(noiseData*0. + rayPos.yzxz*0.0 - vec4(0.0, fTime0_X*0.017, 0.0, 0.0), 0., Texture0).xyz;
      //float noiseVal = noiseData2.r * 0.5;

      // base head
      float sphere = abs(length(tmpPos - vec3(0.0, 2.0*parameters[0][1], 0.0)) - totalSize + 0.3) - firstNoise;
      float sphere2 = abs(length(tmpPos - vec3(0.0, 2.0*parameters[0][1], 0.0)) - totalSize - 3.*firstNoise + 1.5);
      
      float implicitVal;
      vec3 colMult;
      
      colMult = vec3(0.8, 0.3, 0.1);
    
      float outer = min(sphere2, sphere2)-2.8*firstNoise;
      //outer = max(outer, min(outer, 0.) + 5.*noiseVal);
      outer = max(outer, min(outer, 0.));
      
      //implicitVal = min(sphere, outer) - length(tmpPos) * 0.2+1.0 + totalSize * 0.5 - 0.5;
	  implicitVal = min(sphere, outer) - tmpPos.y * 0.6 / max(1.0, totalSize) +
		tmpPos.y*tmpPos.y*0.01 +
		totalSize * 0.2 - 0.9;

	  implicitVal += 1.2 * parameters[0][1];
	  implicitVal = min(15.0, implicitVal);
      
      //totalColor += vec3(1./50., 1./70., 1./90.) * 0.1 * (4.+1.*exp2(4.*noise(vec4(fTime0_X*0.2), 0., Texture0)).r);
	  //totalColor += vec3(1./50., 1./70., 1./90.) * 0.3;
	  totalColor += vec3(1./50., 1./65., 1./90.) * 0.3 / (stepSize+0.5) * 0.5;
      //totalDensity += 1./1000.;
      if (implicitVal < 0.0)
      {
         float localDensity = clamp(0.0 - 0.1*implicitVal, 0.0, 1.0);
         //totalColor = totalColor + (1.-totalDensity) * color(rayPos*0.4, noiseData2, colMult) * localDensity;
         totalColor = totalColor + (1.-totalDensity) * color(rayPos*.4, vec3(0.), colMult) * localDensity;
         totalDensity = totalDensity + (1.-totalDensity) * localDensity;
      }
      
      stepSize = (implicitVal) * 0.2;
      stepSize = 0.002 + smoothstep(0.0, 4.0, stepSize) * 4.;
      //stepSize = max(0.1, stepSize);
      //if (stepSize < 1.0) stepSize = stepSize * stepSize;
      rayPos += rayDir * stepSize;
   }
   
   //float grad = normalize(rayPos).y;
   float grad = objectPosition.y * 0.5 + 0.5;
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));
   
   gl_FragColor = vec4(totalColor-vec3(0.0,0.0,0.0), 1.0);
}