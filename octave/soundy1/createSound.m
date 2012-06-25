function R = createSound(frequencies, timing)
% R = createSound(frequencies, [timing])
%
% Creates the sound from a sound definition
% Its frequency in 1/60 log2-domain (integral),
% Each column is one line of sound.
% The timing is in ticks

% So, a tick is ? samples... This may change.
tick = 4096;
if (nargin < 2)
	timing = 4;
end

% The length of one sound is 4 ticks. For now.
soundLength = 8*tick;
attack = 256;
logAmp = 12;

% I generate the envelope using the distance2error
t = (1:soundLength)';
envelope = distance2error(t, 0, attack, 1, soundLength);

R = zeros(soundLength + tick.*size(frequencies,1).*timing, 1);
for k = 1:size(frequencies, 1)
	for l = 1:size(frequencies, 2)
		soundStart = (k-1) .* timing .* tick;
		amplitude = 2.^logAmp ./ 32768;
		frequency = tone2freq(frequencies(k, l));
		carrier = sin(t./44100.*2.*pi.*frequency);
		R(soundStart+1:soundStart+soundLength) = ...
			R(soundStart+1:soundStart+soundLength) + ...
			carrier .* amplitude .* envelope;
	end
end