% well... we start with pitch
%doPitch
% Now I've got pitch...

% I assume that I have 100% voiced => energy from whatever.
lspec = log(abs(spec));

if (0)
  start1 = 1;
  end1 = ceil(barkBorders(9)./44100.*FFTLength)+1;
  start2 = end1+1;
  end2 = ceil(barkBorders(17)./44100.*FFTLength)+1;
  start3 = end2+1;
  end3 = ceil(barkBorders(25)./44100.*FFTLength)+1;

  % encode lower 8 bands (only y for now):
  dctval = dct(lspec(start1:end1, :));
  dctval(9:end, :) = zeros(size(dctval(9:end, :)));
  lspec(start1:end1, :) = idct(dctval);
  % encode middle 8 bands (only y for now):
  dctval = dct(lspec(start2:end2, :));
  dctval(9:end, :) = zeros(size(dctval(9:end, :)));
  lspec(start2:end2, :) = idct(dctval);
  % encode last 8 bands (only y for now):
  dctval = dct(lspec(start3:end3, :));
  dctval(9:end, :) = zeros(size(dctval(9:end, :)));
  lspec(start3:end3, :) = idct(dctval);
  
  lspec(end3+1:end, :) = ones(size(lspec(end3+1:end, :))) * min(min(lspec));  
end

% Go to 16 bands:
for k = 1:2:16
  barkBorders16(k) = barkBorders((k-1)*3/2+1);
  barkBorders16(k+1) = 0.5 * (barkBorders((k-1)*3/2+2) + barkBorders((k-1)*3/2+3));
end
barkBorders16(17) = barkBorders(25);
% lspec to 16 bands:
for k = 1:16
  startP = floor(barkBorders16(k)./44100*FFTLength) + 1;
  endP = floor(barkBorders16(k+1)./44100*FFTLength) + 1;
  lspec(startP:endP, :) = repmat(mean(lspec(startP:endP, :)), endP-startP+1, 1);
end
lspec(floor(barkBorders16(k+1)./44100*FFTLength)+2:end, :) = min(min(lspec));

% copy lspec:
lspec(FFTLength/2+2:end, :) = flipud(lspec(2:FFTLength/2, :));

% spectral flatness for each band: TODO: use more than one band!
spflat = zeros(length(barkBorders)-1, size(lspec, 2));
for k = 1:length(barkBorders)-1
  stp = max(1, k-1);
  stp2 = min(length(barkBorders), k+2);
  startP = floor(barkBorders(stp)./44100.*FFTLength)+1;
  endP = ceil(barkBorders(stp2)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  spflat(k, :) = prod(abs(spec(startP:endP, :)).^(1/numP)) ./ ...
                 mean(abs(spec(startP:endP, :)));
end
% interpolate spectral flatness
voicedness = zeros(size(lspec));
for k = 1:length(barkBorders)-1
  startP = floor(barkBorders(k)./44100.*FFTLength)+1;  
  endP = ceil(barkBorders(k+1)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  voicedness(startP:endP, :) = 1 - repmat(spflat(k, :), numP, 1);
end
% encode spectral flatness: (auf 24x24?)
% encode lower 8 bands (only y for now):
dctval = dct(voicedness(start1:end1, :));
dctval(5:end, :) = zeros(size(dctval(5:end, :)));
voicedness(start1:end1, :) = idct(dctval);
% encode middle 8 bands (only y for now):
dctval = dct(voicedness(start2:end2, :));
dctval(5:end, :) = zeros(size(dctval(5:end, :)));
voicedness(start2:end2, :) = idct(dctval);
% encode last 8 bands (only y for now):
dctval = dct(voicedness(start3:end3, :));
dctval(5:end, :) = zeros(size(dctval(5:end, :)));
voicedness(start3:end3, :) = idct(dctval);
% encode in time domain:
for k = 1:8:size(voicedness, 2)-7
  dctval = dct(voicedness(:, k:k+7)')';
  dctval(:, 5:end) = zeros(size(dctval(:, 5:end)));
  voicedness(:, k:k+7) = idct(dctval')';
end
% copy spectral flatness
voicedness(FFTLength/2+2:end, :) = flipud(voicedness(2:FFTLength/2, :));

% source: random noise
source = randn(size(orig));
% filter with lspec:
sp2 = spgram(source, frameStep);
sp2 = sp2 .* exp(lspec) .* (1 - voicedness);
unvoiced = reconstruct(sp2);

markers = fromAvgMarkers(pitch, frameStep);
source = pulsetrain(markers, size(orig));
% filter with lspec:
sp2 = spgram(source, frameStep);
sp2 = sp2 .* exp(lspec) .* (voicedness);
voiced = reconstruct(sp2);
