texSize = 256
rbb = rand(texSize, 4) - 0.5;
rl = sqrt(sum(rbb'.*rbb'))';
rb = rbb ./ repmat(rl, 1, 4);

for k = 1:8:64
	r = filtfilt(hamming(k), 1, rb);
	rl = sqrt(sum(r'.*r'))';
	rn = r ./ repmat(rl, 1, 4);
	Rn1 = repmat(rn(:,1), 1, texSize) .* repmat(rn(:,2)', texSize, 1);
	Rn2 = repmat(rn(:,2), 1, texSize) .* repmat(rn(:,3)', texSize, 1);
	Rn3 = repmat(rn(:,3), 1, texSize) .* repmat(rn(:,4)', texSize, 1);
	Rn4 = repmat(rn(:,4), 1, texSize) .* repmat(rn(:,1)', texSize, 1);
	Rn = Rn1+Rn2+Rn3+Rn4;
	figure
	imagesc(Rn)
end