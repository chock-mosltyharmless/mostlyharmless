function r = errormetric(A, B)
% r = errormetric(A, B)
%
% Computes the distance of two sounds A and B
% A sound A is represented by
% Its frequency in 1/60 log2-domain (integral),
% Its log-amplitude (loudness)
% Its start-time (in ticks, integral)
% For now I only care about the frequency

% The number of overtones of each sound to
% take into account for similarity measures
numOvertones = 12;

% calculate frequencies of tone+overtones;
freqA = tone2freq(A(1)) .* (1:numOvertones)';
freqB = tone2freq(B(1)) .* (1:numOvertones)';
toneA = freq2tone(freqA);
toneB = freq2tone(freqB);

% sum the frequency error
% TODO: Estimate the parameters doing some tests...
r = 0;
for (k = 1:numOvertones)
	for (l = 1:numOvertones)
		toneDist = abs(toneA(k) - toneB(l));
		toneErr = distance2error(toneDist, ...
			1.0, 4.0, 16.0, 16.0);
		r = r + toneErr;
	end
end

