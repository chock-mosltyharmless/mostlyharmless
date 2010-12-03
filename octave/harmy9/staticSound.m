function R = staticSound(len, parameters)
% function R = staticSound(len, parameters)
%
% Generates a static modulated sound. The sound
% is not necessarily loopable (clicks).
% The length is in samples
% parameters is a strcuture that contains the
% following fields:
% - modFreq: The frequency of the modulation, in Hz
% - frequency: Currently a static value, in Hz
% - amplitude: log2-amplitude spectrum
% Note: Sampling rate is always 44100 Hz

samplingRate = 44100;

R = zeros(len, 1);

t = (0:len-1)';

% Generate frequency modulation function
modFun = zeros(len, 1);
freqMod = parameters.frequency;
for m = 0:length(freqMod)-1
	modFreq = m * parameters.modFreq;
	modFun = modFun + freqMod(m+1) * ...
		cos((2.0 * pi * modFreq / samplingRate)  * t);
end
	
% Generate sin function with given frequency
phase = 0.0;
base = zeros(len, 1);
for m = 1:len
	freq = modFun(m);
	base(m) = sin(phase);
	phase = phase + 2.0 * pi * freq / samplingRate;
end

% Generate amplitude modulation function
modFun = zeros(len, 1);
ampMod = parameters.amplitude;
for m = 0:length(ampMod)-1
	modFreq = m * parameters.modFreq;
	modFun = modFun + ampMod(m+1) * ...
		cos((2.0 * pi * modFreq / samplingRate) * t);
end
modFun = 2.^modFun; % Go to linear domain

% amplitude-modulate the sound

R = modFun.*base;
