% well... we start with pitch
%doPitch
% Now I've got pitch...
%chosenPitch = 3; Choose the one you see fit. Often 5!

% Constants for psychoacoustics:
psychoacousticFreq = 20.0;
psychoacousticPreTime = 20.0;
psychoacousticPostTime = 20.0;
hearingThreshold = -12;

% Constants for VQ:
voicedVQ = 0.05;
voicedVQx = 0.0;
voicedVQy = 0.0;
voicedMin = 0.025;
energyVQ = 0.05;
energyVQx = 0.0;
energyVQy = 0.0;
energyMin = 0.025;

% Add entries so that it adds up to 16...
spec = spgram(orig, FFTLength, frameStep);
numFrames = size(spec, 2);
missing = 0;
if (mod(numFrames, 16) ~= 0)
  missing = 16 - mod(numFrames, 16);
end
numFrames = numFrames + missing;
spec = [spec, repmat(spec(:, end), 1, missing)];

% I assume that I have 100% voiced => energy from whatever.
lspecBB = zeros(16, numFrames); % I need some more...
lspec = log2(abs(spec));

% Go to 16 bands:
for k = 1:2:16
  barkBorders16(k) = barkBorders((k-1)*3/2+1);
  barkBorders16(k+1) = 0.5 * (barkBorders((k-1)*3/2+2) + barkBorders((k-1)*3/2+3));
end
barkBorders16(17) = barkBorders(25);
% lspec to 16 bands:
for k = 1:16
  startP = floor(barkBorders16(k)./44100*FFTLength) + 1;
  endP = floor(barkBorders16(k+1)./44100*FFTLength);
  disp(startP)  
  lspecBB(k, :) = log2(mean(abs(spec(startP:endP, :))));
  lspecBB(k, :) = max(repmat(hearingThreshold, 1, numFrames), lspecBB(k, :));
  lspec(startP:endP, :) = repmat(lspecBB(k, :), endP-startP+1, 1);  
end
lspec(floor(barkBorders16(k+1)./44100*FFTLength)+2:end, :) = min(min(lspec));

% copy lspec:
lspec(FFTLength/2+2:end, :) = flipud(lspec(2:FFTLength/2, :));

% generate source spectrum:
markers = fromAvgMarkers(pitch(:, chosenPitch), frameStep);
source = pulsetrain(markers, size(orig));
sourceSpec = spgram(source, FFTLength, frameStep);
sourceSpec = [sourceSpec, repmat(sourceSpec(:, end), 1, missing)];
sourceSp = sourceSpec ./ repmat(max(abs(sourceSpec)), FFTLength, 1);

if (0)
% calculate multiplied spec:
multSpec = abs(spec) .* abs(sourceSp);
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

if(0)
% OK, spectral flatness again...
spflat = abs(spec(2:ceil(size(spec, 1) ./ 4), :));
spflat = 2.^(sum(log2(spflat)) ./ size(spflat, 1)) ./ mean(spflat);
spflat = 1 - repmat(spflat, 16, 1);
% Go to extremes:
spflat = spflat .^ 2;
spflat = 0.5 - 0.5 * cos(spflat * pi);

% Do the simple wavelet on voicedness:
spflatWavelet = waveletEncode(spflat, voicedVQ, voicedVQx, voicedVQy, voicedMin);
spflat = waveletDecode(spflatWavelet);

% Go back to 0..1
spflat = max(zeros(size(spflat)), spflat);
spflat = min(ones(size(spflat)), spflat);
end
%spflat = ones(size(spflat));

% Yet another spectral flatness:
spflat = zeros(16, size(spec, 2));
RcepsCopy = [RcepsCopy; repmat(RcepsCopy(end, :), missing, 1)];
for k = 1:size(spflat, 2)
  startP = floor(pitch(:, chosenPitch)/4);
  spflat(:, k) = max(RcepsCopy(k, startP:end));
end
%heuristic...
spflat = max(zeros(size(spflat)), spflat*1.5-0.5);

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

if(0)
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
maxPsychoLspecBB = log2(2.^(psychoLspecBB) ./ ssEner .* 15);

% Now I encode the lspec... simple version first:
% Do the simple wavelet on voicedness:
lspecWavelet = waveletEncode(psychoLspecBB, energyVQ, energyVQx, energyVQy, energyMin);
lspecBB = waveletDecode(lspecWavelet);
end

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

% adjust according to pitch
avgPitch = median(pitch(:, chosenPitch));
%voicedness = voicedness * sqrt(avgPitch) ./ (1 - voicedness + voicedness * sqrt(avgPitch));
voicedness = voicedness * avgPitch ./ (1 - voicedness + voicedness * avgPitch);

% source: random noise
source = randn(size(orig));
% filter with lspec:
sp2 = spgram(source, FFTLength, frameStep);
sp2 = [sp2, zeros(FFTLength, missing)];
sp2 = sp2 .* 2.^(lspec) .* (1 - voicedness);
unvoiced = reconstruct(sp2, frameStep);

markers = fromAvgMarkers(pitch(:, chosenPitch), frameStep);
source = pulsetrain2(markers, size(orig));
% filter with lspec:
sp2 = spgram(source, FFTLength, frameStep);
sp2 = [sp2, zeros(FFTLength, missing)];
sp2 = sp2 .* 2.^(lspec) .* (voicedness);
voiced = reconstruct(sp2, frameStep);
