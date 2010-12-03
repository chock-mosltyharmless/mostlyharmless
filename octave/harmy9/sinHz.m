function R = sinHz(len, freq, sampFreq)
% R = sinHz(len, freq[, sampFreq])
%
% Generates a sinus function with a give frequency in Hz.
% len: Length of the generated time series in samples
% freq: Frequency of the sinus function in Hz
% sampFreq [optional]: sampling frequency

if (nargin < 3)
	sampFreq = 44100;
end

t = (0:len-1)';
R = sin((2.0 * pi * freq / sampFreq) * t);
