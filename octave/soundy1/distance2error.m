function r = distance2error(d, startErrVal, maxErrPos, ...
							maxErrVal, endErrPos)
% r = distance2error(d, startErrVal, maxErrPos,
%                    maxErrVal, endErrPos)
%
% Computes the error given a distance and the
% parameters of the transformation function.
% The function has a non-zero error for a distance
% of 0, then raises until the max and then goes
% down to 0. The function is realized by means
% of a linear and a cosine function.

r = zeros(size(d));

if (any(d < 0))
	disp('ERROR: Negative distance!');
	return;
end

% start positions
pos = find(d >= 0 & d <= maxErrPos);
if (length(pos) > 0)
	t = d(pos) ./ maxErrPos;
	r(pos) = startErrVal + t .* (maxErrVal - startErrVal);
end
	
% max-end
pos = find(d > maxErrPos & d <= endErrPos);
if (length(pos) > 0)
	t = (d(pos) - maxErrPos) ./ (endErrPos - maxErrPos);
	r(pos) = 0.5 + 0.5 * cos(t * pi);
	r(pos) = r(pos) * maxErrVal;
end
	
% end
pos = find(d > endErrPos);
if (length(pos) > 0)
	r(pos) = 0;
end