function R = segment(parameters1, parameters2, length, ...
					 miniLength)
% function R = segment(parameters1, parameters2, length,
%                      [miniLength])
%
% Generates one segment of sound for one line.
% The length is the number of mini-blocks to generate,
% miniLength [optional] is the length of one mini block
% in samples.
% Parameters are structures that contain the following
% fields:
% 

samplingFrequency = 44100;

if (nargin < 4)
	% The fastest play frequency is 16 Hz?
	miniLength = floor(samplingFrequency / 16);
end

R = zeros(length*miniLength, 1);