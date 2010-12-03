function R = barkFilterCoefs(FFTLength, barkCenters)
% R = barkFilterCoefs
%
% Calculate the coefficients that are
% used to emulate a bark filter bank.
% FFTLength is the length of the FFT used.
% SampleRate must be 44100

  if (nargin < 2)
    barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
                   1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
                   4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
  end

  numBands = length(barkCenters) - 2;
  R = zeros(FFTLength, numBands);
  
  % Transfer barkCenters to FFT length domain
  barkCenters = floor((barkCenters ./ 44100 .* FFTLength) + 0.5) + 1;
  
  for k = 1:numBands
    ind = barkCenters(k):barkCenters(k+1);
    R(ind, k) = (0:length(ind)-1) / (length(ind)-1);
    ind = barkCenters(k+1):barkCenters(k+2);
    R(ind, k) = (length(ind)-1:-1:0) / (length(ind)-1);
  end
  
  % copy R for dual whatever.
  source = 2:FFTLength/2;
  dest = FFTLength:-1:FFTLength/2+2;
  R(dest, :) = R(source, :);

  % normalize R
  R = R ./ repmat(sum(R), size(R, 1), 1);