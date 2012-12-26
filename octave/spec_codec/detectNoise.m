function [noisyness, average, deviation] = detectNoise(block)
% [noisyness, average, deviation] = detectNoise(block)
%
% Given a block of numbers, this algorithm tries to detect
% whether it consists of noise or not. It returns this in
% a range between 0 and 1. Additionally, the algorithm
% Tells the average value of the noise and the maximum deviation.
%
% Let me try an implementation where I check every 3x3 subblock
% concerning its mean value and its standard deviation. If all blocks
% have about the same, I assume it's just noise.

sizeX = size(block, 2);
sizeY = size(block, 1);

average = mean(mean(block));
deviation = 0.5 * (max(max(block)) - min(min(block)));

% I assume that this is just noise and then try to prove
% it otherwise.
noisyness = 1;

subblockSize = 4
numOutliers = 2

subblockMeans = zeros(sizeY-subblockSize+1, sizeX-subblockSize+1);
subblockDevs = subblockMeans;
for x = 1:(sizeX-subblockSize+1)
	for y = 1:(sizeY-subblockSize+1)
		subblock = block(y:(y+subblockSize-1),x:(x+subblockSize-1));
		sortedSubblock = sort(subblock(:));
		partBlock = sortedSubblock(numOutliers+1:end-numOutliers);
		subblockMeans(y, x) = mean(partBlock);
		subblockDevs(y, x) = max(partBlock) - min(partBlock);
	end
end

noisyness  = subblockDevs;
average = subblockMeans;

% This seem to be suitable values, but they largely depend on the
% size of the dataset. Maybe if I use a percentage of outliers????
% Both, deviation of deviation and deviation of means seem to hold information.
mean(subblockDevs(:))  %% This is the expected deviation on large scale.
max(subblockDevs(:)) - min(subblockDevs(:)) %% This is the deviation of the deviation on large scale
max(subblockMeans(:)) - min(subblockMeans(:))  %% This is the deviation of the means on large scale