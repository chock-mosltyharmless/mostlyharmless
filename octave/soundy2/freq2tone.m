function t = freq2tone(f)
% t = freq2tone(f)
%
% Converts a frequency from the frequency(Hz)
% domain into the 1/60 log2 tone domain

% Note: The lowest tone (0) is equal to 16 Hz.
t = 60 .* log2(f ./ 16);