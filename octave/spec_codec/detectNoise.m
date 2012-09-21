function [noisyness, average, deviation] = detectNoise(block)
% [noisyness, average, deviation] = detectNoise(block)
%
% Given a block of numbers, this algorithm tries to detect
% whether it consists of noise or not. It returns this in
% a range between 0 and 1. Additionally, the algorithm
% Tells the average value of the noise and the maximum deviation.
%
% I don't know yet how it shall be implemented, so...

average = mean(mean(block));
deviation = 0.5 * (max(max(block)) - min(min(block)));

% I assume that this is just noise and then try to prove
% it otherwise.
noisyness = 1;

% The distribution of values should be flat. How do I check this?
values = block(:);
numValues = length(values);
if (numValues > 12)
	distribution = hist(values, numValues / 4); % average 4 values per bin.
	size(distribution)
	plot(distribution)
	% only check for values that are too large...
	toomany = min(0.0, max(distribution) - 4);
	modifier = 1.0 / (toomany+1)^0.2;
	noisyness = noisyness * modifier;
end