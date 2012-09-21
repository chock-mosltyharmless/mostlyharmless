function R = pitch2FFTIndex(A, FFTLength)
% R = pitch2FFTIndex(A, FFTLength)
%
% Converts pitch step sizes like estimated in doPitch
% to indices in the FFT spectrum.
% This function does not apply flooring, it returns
% floating point numbers.
%
% Note that I add 1 for the constant FFT index. So
% You may not simply multiply the returned values
% in order to get higher FFT indices.

R = FFTLength ./ A + 1;