% Check some artificial reverberation

samplingRate = 44100
duration = 5 % in seconds
signal = zeros(samplingRate*duration, 1);
signal(1) = 1.0;

% The delay line specification
delayLineLength = 256
delayLineTaps = 4
delayLineAmp = 0.22 * ones(delayLineTaps, 1)
tapPos = rand(delayLineTaps, 1) * (delayLineLength-1);
tapPos = floor(tapPos + 1)
delayLine = zeros(delayLineLength, 1);
delayLinePos = 1;

result = zeros(size(signal));
for k = 1:4000%length(signal)
	taps = mod(delayLinePos + tapPos, delayLineLength) + 1;
	delayLine(taps) = delayLine(taps) + signal(k) +...
		delayLine(delayLinePos) * delayLineAmp;
	result(k) = delayLine(delayLinePos) + signal(k);
	delayLine(delayLinePos) = 0;
	delayLinePos = mod(delayLinePos, delayLineLength) + 1;
end