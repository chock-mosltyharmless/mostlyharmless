function R = melfilter(A, FFTLength, frameStep, numBands)
## R = melfilter(A, FFTLength, frameStep, numBands)
##
## TODO: Add some sort of smoothing to the FFT

% Build up the triangle filter centers
numCenters = numBands;
maxMel = freq2mel(20000);
minMel = freq2mel(0);
melCenters = ((0:numCenters-1)' + minMel) * (maxMel - minMel) ./ (numCenters-1);
freqCenters = mel2freq(melCenters);
fftCenters = floor(freqCenters ./ 44100 .* FFTLength + 0.5)

triangleFilters = zeros(FFTLength/2, numBands);
for band = 2:numBands
  startP = fftCenters(band-1)+1;
  midP = fftCenters(band)+1;
  triangleFilters(startP:midP, band) = ((startP:midP)' - startP + 1) ./ (midP-startP+1);
end
for band = 1:numBands-1
  midP = fftCenters(band)+1;
  endP = fftCenters(band+1)+1;
  triangleFilters(midP:endP, band) = ((endP:-1:midP)' - midP + 1) ./ (endP-midP+1);
end

% Normalize triangle filters
triangleFilters = triangleFilters ./ repmat(sum(triangleFilters, 1), size(triangleFilters, 1), 1);

% FFT of A
spec = abs(spgram(A, FFTLength, frameStep));
spec = spec(1:FFTLength/2, :);
numFrames = size(spec, 2);
% Mel
R = zeros(numBands, numFrames);
for band = 1:numBands
  R(band, :) = sum(repmat(triangleFilters(:, band), 1, numFrames) .* spec);
end
