function R = klatt(x, sigLength, parameter)
% R= klatt(x, sigLength, parameter)
%
% calculate klatt value at some point x (between 0 and sigLength).
% The parameter defines the length
% of the opening of the vocal folds. (e.g. 0.5)

t = x ./ sigLength;
amplitude = 1 / parameter / parameter / parameter / 0.15;
R = amplitude * parameter .* t .* t - amplitude .* t .* t .* t;
R = max(R, zeros(size(t)));
