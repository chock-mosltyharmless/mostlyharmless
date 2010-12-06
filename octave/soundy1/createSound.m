function R = createSound(A)
% R = createSound(A)
%
% Creates the sound from a sound definition
% A sound A is represented by
% Its frequency in 1/60 log2-domain (integral),
% Its log-amplitude (loudness)
% Its start-time (in ticks, integral)

% So, a tick is ? samples... This may change.
tick = 4096;

% The length of one sound is 4 ticks. For now.
soundLength = 4*tick;
attack = 256;

% I generate the envelope using the distance2error
t = (1:soundLength)';
envelope = distance2error(t, 0, attack, 1, soundLength);

R = zeros(soundLength + tick.*max(A(3,:)), 1);
for k = 1:size(A, 2)
	amplitude = 2.^A(2, k) ./ 32768;
	frequency = tone2freq(A(1, k));
	carrier = sin(t./44100.*2.*pi.*frequency);
	soundStart = A(3,k) .* tick;
	R(soundStart+1:soundStart+soundLength) = ...
		R(soundStart+1:soundStart+soundLength) + ...
		carrier .* amplitude .* envelope;
end