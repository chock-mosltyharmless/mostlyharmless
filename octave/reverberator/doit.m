% Check some artificial reverberation

samplingRate = 44100
duration = 5 % in seconds
signal = zeros(samplingRate*duration, 1);
signal(1) = 1.0;

% Delay between 0 und 200 ms.
numReverbs = 16;
maxSampleDelay = floor(0.1 * samplingRate) + 1;
minSampleDelay = floor(0.0 * samplingRate) + 1;
randSampleDelay = maxSampleDelay - minSampleDelay;
delays = floor(rand(numReverbs, 1) * randSampleDelay) + minSampleDelay
%rotAngles = (rand(numReverbs, 1)-0.5) * 0.2 * pi + pi/4
%rotAngles = rand(numReverbs, 1) * 2.0 * pi
amplitudes = 2.0 * (rand(numReverbs, 2) - 0.5)
gains = ones(numReverbs, 1) * [1.0 0.97];
amplitudes = amplitudes .* gains;
lowpassFractions = 0.5 * ones(numReverbs, 1);

signal2D = [signal, signal];
result2D = signal2D;
delayed = zeros(size(result2D));
	
for k = 1:numReverbs
	delayed(:, 2) = delay(result2D(:, 2), delays(k), 0.0);
	%delayed = filter([1.0 - lowpassFractions(k), lowpassFractions(k)], 1.0, delayed);
	delayed = filter(0.8, [1.0, -0.5], delayed);
	% Copy the remember line to the copy part
	delayed(:, 1) = delayed(:, 2) * amplitudes(k, 1);
	delayed(:, 2) = delayed(:, 2) * amplitudes(k, 2);
	%delayed = rotate(delayed, rotAngles(k));
	%result2D = result2D + delayed * gains(k);
	result2D = result2D + delayed;
end

% diagnostics
downsampled = decimate(abs(result2D(:,1)), 100);
[S, f, t] = specgram(result2D(:,1), 2048);
S = log(abs(S) + 1e-7);