function R = delay(A, samples, lowpassFraction)
% R = delay(A, samples[, lowpassFraction])
%
% Delays a sound (may be n-dimensional) by padding
% zeros at the beginning. The length of the signal
% stays unchanged (the signal is cropped).

if (nargin < 3)
	lowpassFraction = 0.0;
end

dimension = size(A, 2);
numSamples = size(A, 1);

R1 = [zeros(samples, dimension);...
	  A(1:numSamples - samples, :)];
R2 = [zeros(samples+1, dimension);...
	  A(1:numSamples - samples - 1, :)];
	  
R = (1.0 - lowpassFraction) * R1 + ...
	lowpassFraction * R2;