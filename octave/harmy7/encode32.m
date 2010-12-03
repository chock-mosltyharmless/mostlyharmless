% well... we start with pitch
%doPitch
% Now I've got pitch...
%chosenPitch = 3; Choose the one you see fit. Often 5!

% Constants for psychoacoustics:
psychoacousticFreq = 2.0;
psychoacousticPreTime = 2.0;
psychoacousticPostTime = 0.5;
hearingThreshold = -8;

% Constants for VQ:
%do = dont.
voicedVQ = 1;
voicedVQx = 0.0;
voicedVQy = 0.0;
energyVQ = 0.1;
energyVQx = 0.0;
energyVQy = 0.0;

% Add entries so that it adds up to 16...
spec = spgram(orig, frameStep);
numFrames = size(spec, 2);
missing = 0;
if (mod(numFrames, 16) ~= 0)
  missing = 16 - mod(numFrames, 16);
end
numFrames = numFrames + missing;
spec = [spec, repmat(spec(:, end), 1, missing)];

% I assume that I have 100% voiced => energy from whatever.
lspecBB = zeros(16, numFrames); % I need some more...
lspec = log(abs(spec));

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
  lspecBB(k, :) = log(mean(abs(spec(startP:endP, :))));
  lspecBB(k, :) = max(repmat(hearingThreshold, 1, numFrames), lspecBB(k, :));
  lspec(startP:endP, :) = repmat(lspecBB(k, :), endP-startP+1, 1);  
end
lspec(floor(barkBorders16(k+1)./44100*FFTLength)+2:end, :) = min(min(lspec));

% copy lspec:
lspec(FFTLength/2+2:end, :) = flipud(lspec(2:FFTLength/2, :));

% generate source spectrum:
markers = fromAvgMarkers(pitch(:, chosenPitch), frameStep);
source = pulsetrain(markers, size(orig));
sourceSpec = spgram(source, frameStep);
sourceSpec = [sourceSpec, repmat(sourceSpec(:, end), 1, missing)];

% calculate multiplied spec:
multSpec = abs(spec) .* abs(sourceSpec);

if (0)
% spectral flatness for each band: It's rather the energy of multSpec vs. spec.
spflat = zeros(length(barkBorders16)-1, size(lspec, 2));
for k = 1:length(barkBorders16)-1
  %stp = 1;
  %stp2 = length(barkBorders16);
  %stp = max(1, k-1);
  %stp2 = min(length(barkBorders16), k+2);
  stp = k;
  stp2 = k+1;
  startP = floor(barkBorders16(stp)./44100.*FFTLength)+1;
  endP = ceil(barkBorders16(stp2)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  spflat(k, :) = sum(multSpec(startP:endP, :)) ./ ...
                 sqrt(sum(abs(spec(startP:endP, :)) .* abs(spec(startP:endP, :)))) ./ ...
                 sqrt(sum(abs(sourceSpec(startP:endP, :)) .* abs(sourceSpec(startP:endP, :))));
end
% heuristic:
spflat = spflat .^ 2;
% average per frame...
%spflat = repmat(mean(spflat), size(spflat, 1), 1);
end

% OK, spectral flatness again...
spflat = abs(spec(2:ceil(size(spec, 1) ./ 4), :));
spflat = exp(sum(log(spflat)) ./ size(spflat, 1)) ./ mean(spflat);
spflat = 1 - repmat(spflat, 16, 1);
% Go to extremes:
spflat = spflat .^ 2;
spflat = 0.5 - 0.5 * cos(spflat * pi);

% Do the simple wavelet on voicedness:
spflatWavelet = waveletEncode(spflat, voicedVQ, voicedVQx, voicedVQy);
spflat = waveletDecode(spflatWavelet);

% Go back to 0..1
spflat = max(zeros(size(spflat)), spflat);
spflat = min(ones(size(spflat)), spflat);

%spflat = ones(size(spflat));

% Here I would do the psychoacoustics: I check the loudness of the reconstructed!
% I have sourceSpec, which is the spec of the voiced source
% I have to append unvoiced stuff.
% interpolate spectral flatness
voicedness = zeros(size(lspec));
for k = 1:length(barkBorders16)-1
  startP = floor(barkBorders16(k)./44100.*FFTLength)+1;  
  endP = ceil(barkBorders16(k+1)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  voicedness(startP:endP, :) = repmat(spflat(k, :), numP, 1);
end
% copy spectral flatness
voicedness(FFTLength/2+2:end, :) = flipud(voicedness(2:FFTLength/2, :));

% I get the energy of voiced+unvoied source
sourceSpec = voicedness .* abs(sourceSpec) + 10 * ones(size(sourceSpec)) .* (1-voicedness);
% Go to bark domain:
ssEner = zeros(16, numFrames);
for k = 1:length(barkBorders16)-1
  startP = floor(barkBorders16(k)./44100.*FFTLength)+1;
  endP = ceil(barkBorders16(k+1)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  ssEner(k, :) = mean(sourceSpec(startP:endP, :));
end

% go to psychoacoustic lspec...
psychoLspecBB = lspecBB;
% Frequency up:
for k = 2:16
  psychoLspecBB(k, :) = max(psychoLspecBB(k, :), psychoLspecBB(k-1, :) - psychoacousticFreq);
end
% Frequency down:
for k = 15:-1:1
  psychoLspecBB(k, :) = max(psychoLspecBB(k, :), psychoLspecBB(k+1, :) - psychoacousticFreq);
end
% Post time
for k = 2:numFrames
  psychoLspecBB(:, k) = max(psychoLspecBB(:, k), psychoLspecBB(:, k-1) - psychoacousticPostTime);
end
% Pre time
for k = numFrames-1:-1:1
  psychoLspecBB(:, k) = max(psychoLspecBB(:, k), psychoLspecBB(:, k+1) - psychoacousticPreTime);
end
% maximum values to set is psychoLspecBB ./ ssEner .* average max ssEner
maxPsychoLspecBB = log(exp(psychoLspecBB) ./ ssEner .* 15);

% Now I encode the lspec... simple version first:
% Do the simple wavelet on voicedness:
lspecWavelet = waveletEncode(psychoLspecBB, energyVQ, energyVQx, energyVQy);
lspecBB = waveletDecode(lspecWavelet);

%%% Reconstruction:

% recreate energy:
for k = 1:16
  startP = floor(barkBorders16(k)./44100*FFTLength) + 1;
  endP = floor(barkBorders16(k+1)./44100*FFTLength) + 1;
  lspec(startP:endP, :) = repmat(lspecBB(k, :), endP-startP+1, 1);    
end
% Did that already:
%lspec(floor(barkBorders16(k+1)./44100*FFTLength)+2:end, :) = min(min(lspec));

% copy lspec:
lspec(FFTLength/2+2:end, :) = flipud(lspec(2:FFTLength/2, :));

% interpolate spectral flatness
voicedness = zeros(size(lspec));
for k = 1:length(barkBorders16)-1
  startP = floor(barkBorders16(k)./44100.*FFTLength)+1;  
  endP = ceil(barkBorders16(k+1)./44100.*FFTLength)+1;
  numP = endP-startP+1;  
  voicedness(startP:endP, :) = repmat(spflat(k, :), numP, 1);
end
% copy spectral flatness
voicedness(FFTLength/2+2:end, :) = flipud(voicedness(2:FFTLength/2, :));

% source: random noise
source = randn(size(orig));
% filter with lspec:
sp2 = spgram(source, frameStep);
sp2 = [sp2, zeros(FFTLength, missing)];
sp2 = sp2 .* exp(lspec) .* (1 - voicedness);
unvoiced = reconstruct(sp2);

markers = fromAvgMarkers(pitch(:, chosenPitch), frameStep);
source = pulsetrain(markers, size(orig));
% filter with lspec:
sp2 = spgram(source, frameStep);
sp2 = [sp2, zeros(FFTLength, missing)];
sp2 = sp2 .* exp(lspec) .* (voicedness);
voiced = reconstruct(sp2);
