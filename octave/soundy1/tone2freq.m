function f = tone2freq(t)
% f = tone2freq(t)
%
% Converts a frequency from the 1/60 log2 tone
% domain into the frequency (Hz) domain

% Note: The lowest tone (0) is equal to 16 Hz.
f = 16 * 2.^(t ./ 60);