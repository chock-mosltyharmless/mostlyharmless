function R = createSound(tones)
% R = createSound(tones)
%
% Creates the sound from a sound definition
% Its frequency in 1/60 log2-domain (integral),
% Each column is one element of sound.
% NEW: tones are the frequencies

soundLength = 88200; % For now: simply two seconds
logAmp = 12;

% I generate the envelope using the distance2error
t = (1:soundLength)';

R = zeros(soundLength, 1);
for k = 1:length(tones(:))
	amplitude = 2.^logAmp ./ 32768;
	%frequency = tone2freq(tones(k));
	frequency = tones(k);
	carrier = sin(t./44100.*2.*pi.*frequency);
	carrier = carrier + sin(2.*t./44100.*2.*pi.*frequency)/2;
	carrier = carrier + sin(3.*t./44100.*2.*pi.*frequency)/3;
	carrier = carrier + sin(4.*t./44100.*2.*pi.*frequency)/4;
	carrier = carrier + sin(5.*t./44100.*2.*pi.*frequency)/5;
	carrier = carrier + sin(6.*t./44100.*2.*pi.*frequency)/6;
	R = R + carrier * amplitude;
end