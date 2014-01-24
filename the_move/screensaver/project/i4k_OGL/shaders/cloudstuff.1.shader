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

vec3 color(vec3 pos, vec3 noiseStuff, vec3 colMult)
{   
   float dist = length(pos);
   float color = smoothstep(0.3, 1.2, dist);
   float brightness = smoothstep(-1.8, 0.0, -color);
   float stuffi = noiseStuff.g * noiseStuff.b * 4.0;
   
   colMult -= 0.1*stuffi * (4.1, 10.5, 7.2);
   return 2.*mix(vec3(1.0)-colMult, colMult, color) * brightness + 0.2 * (1.0-brightness);
   //return vec3(0.3, 0.2, 0.1);
}

void main(void)
{  
   float fTime0_X = parameters[0][0];

   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0 / parameters[0][1]));
   //vec3 rayDir = normalize(objectPosition);
   vec3 camPos = vec3(-0.7, 1.0, -6.0 + 3.0 * sin(fTime0_X*0.7));
   
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
   float sceneSize = 12.0;
   vec3 totalColor = vec3(0.);
   float stepSize;
   float totalDensity = 0.0;
   
   for (int stepi = 0; 
        length(rayPos)<sceneSize && totalDensity < 0.95 && stepi < 150;
        stepi++)
   {
      vec3 tmpPos = rayPos;   
   
      vec4 noiseData = 0.3*noise(tmpPos.xzyx*0.03+ vec4(0.0, fTime0_X*0.03, 0.0, 0.0), 2., Texture0);
      noiseData += 0.2 * noise(tmpPos.xzzy*0.07 + noiseData, 2., Texture0);
      noiseData += 0.1 * noise(tmpPos.xzzy*0.2+ vec4(0.0, fTime0_X*0.12, 0.0, 0.0), 2., Texture0);
      float firstNoise = noiseData.r;
      firstNoise = clamp(0.4 - abs(firstNoise), 0.0, 0.4);
      firstNoise *= firstNoise * 3.;      

      noiseData *= 2.0;
      noiseData /= length(tmpPos) + 0.5;

      vec4 noiseData2 = -1.3*noise(noiseData*4.0 + 0.2*tmpPos.yxzx, 1., Texture0);
      noiseData2 += 1.0*noise(noiseData.xywz*9.0, 2., Texture0);
      //noiseData2 += 0.3*noise(noiseData.zwyx*11.0, 3., Texture0);
      noiseData2 *= sin(noiseData.g*15.0);
      float noiseVal = noiseData2.r * 0.5;

      // base head
      //float sphere = length(tmpPos) - 0. - firstNoise;
      float sphere3 = abs(length(tmpPos) - 2.0 - 2.*firstNoise + sin(fTime0_X*0.367) * sin(tmpPos.x + 3.0*fTime0_X)) + .5;
      float sphere2 = abs(length(tmpPos) - 1.2 - 2.*firstNoise + sin(fTime0_X*0.174) * 1.5 * sin(tmpPos.y - 2.7* fTime0_X)) + 0.2;
      
      float implicitVal;
      vec3 colMult;
      
      colMult = vec3(0.8, 0.5, 0.3);
    
      //sphere = min(min(sphere, sphere3 - 1.*firstNoise), sphere2-firstNoise);
      float outer = min(sphere2, sphere3)-1.8*firstNoise;
      outer = max(outer, min(outer, 0.) + 3.*noiseVal);
      
      //implicitVal = min(sphere, outer);
      implicitVal = outer;
      
      totalColor += (vec3(1./60., 1./70., 1./80.) * 2.2 - 0.05*firstNoise) / (stepSize+0.5) * 0.5;
      totalDensity += 1./400.;
      if (implicitVal < 0.0)
      {
		 vec3 localColor = color(rayPos*0.4, noiseData2.xyz, colMult);
		 localColor -= abs(noiseData2.g * 8.);
		 localColor -= 0.5;
         float localDensity = clamp(0.0 - max(0.3-20.*abs(noiseVal),0.05)*implicitVal, 0.03, 1.0);
         //localDensity = 1.0;
         totalColor = totalColor + (1.-totalDensity) * localColor * localDensity;
         totalDensity = totalDensity + (1.-totalDensity) * localDensity;
      }
      
      stepSize = (implicitVal) * 0.6;
      stepSize = 0.05 + smoothstep(0.0, 2.0, stepSize) * 2.;
      //stepSize = max(0.1, stepSize);
      //if (stepSize < 1.0) stepSize = stepSize * stepSize;
      rayPos += rayDir * stepSize;
   }
   
   //float grad = normalize(rayPos).y;
   float grad = objectPosition.y * 0.5 + 0.5;
   //totalColor *= totalDensity;
   //totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.1,0.2) + (1.-grad)*vec3(0.0,0.2,0.3));
   //totalColor += (1.-totalDensity) * (grad * vec3(0.3,0.6,1.0) + (1.-grad)*vec3(0.6,0.8,1.0));
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));

   gl_FragColor = vec4(totalColor-vec3(0.0), 1.0);
}