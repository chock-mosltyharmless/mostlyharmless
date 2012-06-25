function R = moveSoundSource(startPos, moveDir, baseSound)
% R = moveSoundSource(startPos, moveDir, baseSound)
%
% Movement is moveDir for each sample.
% Position can be in any dimension

numSamples = length(baseSound);
R = zeros(size(baseSound));
distN = zeros(size(baseSound));

oldPos = startPos;
for k = 1:numSamples-1
	newPos = oldPos + moveDir;
	dist = sqrt(sum(newPos.*newPos));
	distN(k) = dist;
	
	% Get subsample, using distance as the time delay
	basePos = floor(k - dist);
	if (basePos < 1.0)
		R(k) = baseSound(1);
	else
		R1 = baseSound(basePos);
		R2 = baseSound(basePos + 1);
		t = (k - dist) - basePos;
		R(k) = (1.0 - t) * R1 + t * R2;
	end
	
	oldPos = newPos;
end

plot(distN(1:100:end));