uniform sampler2D Texture0;
uniform sampler2D Texture1;
varying vec3 objectPosition;
varying mat4 parameters;

void main(void)
{  
   float fTime0_X = parameters[0][0];
   vec3 noisePos = objectPosition + fTime0_X;
   vec2 noiseVal;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43711.5453);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43717.5453);
   gl_FragColor = 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy);
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43731.5453);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43737.5453);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy);
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 33758.5453);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43285.5233);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43263.5635);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 41241.2413);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 28571.5343);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 33938.2456);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 49245.5456);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43571.5343);
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 38134.3452);
   gl_FragColor += 0.14 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.01 - 0.015;

   float vignette = objectPosition.x*objectPosition.x + objectPosition.y*objectPosition.y;
   vignette = sqrt(vignette);
   gl_FragColor *= 1.2 - vignette * 0.3; // darken
   float meanColor = 0.3 * gl_FragColor.r + 0.59 * gl_FragColor.r + 0.11 * gl_FragColor.b;
   gl_FragColor = 0.2 * vignette * vec4(meanColor) + (1.0 - 0.6 * vignette) * gl_FragColor; // desaturate
}
