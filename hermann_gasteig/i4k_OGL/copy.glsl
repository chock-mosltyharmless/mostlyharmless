uniform sampler2D Texture0; 
varying vec3 objectPosition; 
varying mat4 parameters; 
 
void main(void) 
{   
   float fTime0_X = parameters[0][0]; 
   vec3 noisePos = objectPosition + fTime0_X; 
   vec2 noiseVal; 
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453); 
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453); 
   gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.02; 
 
}