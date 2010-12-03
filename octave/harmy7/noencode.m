% well... we start with pitch
%doPitch
% Now I've got pitch...
chosenPitch = 1; %Choose the one you see fit. Often 1!
%voicednessDrop = 0.01;
voicednessDrop = 0.0;

% Add entries so that it adds up to 1...
spec = spgram(orig, FFTLength, frameStep);
numFrames = size(spec, 2);

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
  %disp(startP)
  lspecBB(k, :) = log2(mean(abs(spec(startP:endP, :))));
  %lspecBB(k, :) = max(repmat(hearingThreshold, 1, numFrames), lspecBB(k, :));
  lspec(startP:endP, :) = repmat(lspecBB(k, :), endP-startP+1, 1);  
end
lspec(floor(barkBorders16(k+1)./44100*FFTLength)+2:end, :) = min(min(lspec));

% copy lspec:
lspec(FFTLength/2+2:end, :) = flipud(lspec(2:FFTLength/2, :));

% Yet another spectral flatness:
spflat = zeros(16, size(spec, 2));
for k = 1:size(spflat, 2)
  startP = floor(pitch(:, chosenPitch)/4);
  spflat(:, k) = max(RcepsCopy(k, startP:end));
end
%heuristic...
spflat = max(zeros(size(spflat)), spflat*1.0-0.0);
%voicedness dropping:
spflat = spflat .* repmat(1 - voicednessDrop * (0:15)', 1, numFrames);


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

%%% Reconstruction:

% pitch to quater-notes:
storePitch = floor((log2(pitch)-3)*24 + 0.5);
storePitch = min(storePitch, 255*ones(size(storePitch)));
storePitch = max(storePitch, zeros(size(storePitch)));
pitch = 2.^(storePitch/24 + 3);

% lspec encode:
storeLSpec = floor((lspecBB+8) * 16);
storeLSpec = min(storeLSpec, 255*ones(size(storeLSpec)));
storeLSpec = max(storeLSpec, zeros(size(storeLSpec)));
lspecBB = storeLSpec / 16 - 8;

% voicedness encode
storeVoicedness = floor((spflat * 255) + 0.5);
spflat = storeVoicedness / 255;

% recreate energy:
lspec = -20 * ones(size(lspec));
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
% The problem here is that the unvoiced energy has sqrt(pitch) more energy
%avgPitch = median(pitch(:, chosenPitch));
%voicedness = voicedness * sqrt(avgPitch) ./ (1 - voicedness + voicedness * sqrt(avgPitch));
%voicedness = voicedness * avgPitch ./ (1 - voicedness + voicedness * avgPitch);
% I also need to modify energy? That's too complicated for me...

% source: random noise
source = randn(size(orig));
% filter with lspec:
sp2 = spgram(source, FFTLength, frameStep);
sp2 = sp2 .* 2.^(lspec) .* (1 - voicedness);
unvoiced = reconstruct(sp2, frameStep);

markers = fromAvgMarkers(pitch(:, chosenPitch), frameStep);
source = pulsetrain2(markers, size(orig));
% filter with lspec:
sp2 = spgram(source, FFTLength, frameStep);
sp2 = sp2 .* 2.^(lspec) .* (voicedness);
voiced = reconstruct(sp2, frameStep);

% write to
writeFile = [filename, '.tga']

image = zeros(34, 256);
image(1:16, 1:size(storeLSpec, 2)) = storeLSpec;
image(17:32, 1:size(storeVoicedness, 2)) = storeVoicedness;
image(33, 1:length(storePitch)) = storePitch';
image = repmat(image, [1, 1, 3]);

tgaWrite(writeFile, image);