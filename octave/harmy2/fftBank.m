function [R, voicedness] = fftBank(A, FFTLength, filterbank)
% [R, voicedness] = fftBank(A, FFTLength, filterbank)
%
% Calculate the spectrogramm of A and calculate the bank energies using filterbank
% (which is calculated w.g. by barkFilterCoefs).

oversample = 8;

voic = [];

% normalize the filterbank.
% This destroys the reconstruction!
for k = 1:size(filterbank, 2)
  filterbank(:, k) = filterbank(:, k) ./ sum(filterbank(:, k));
end

% Overlapping whatever
%FFTstep = FFTLength/2;
%ind = ones(FFTLength, 1) * (0:FFTstep:length(A)-FFTLength);
%ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
%frames = A(ind);

% I calculate the length of the original working stuff...
numFrames = floor((length(A)-FFTLength)/FFTLength*2)+1;

% I am oversampling this oversample times.
FFTstep = FFTLength/2/oversample;
ind = ones(FFTLength, 1) * (0:FFTstep:length(A)-FFTLength);
ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
frames = A(ind);

% apply hamming window
% TODO: I might consider using some chebyshef window here, maybe with
% longer FFT length.
win = hamming(FFTLength);
%win = nuttallwin(FFTLength)';
frames = frames .* (repmat(win, 1, size(frames, 2)));

% calculate fft
frames = abs(fft(frames));

% go to energies?
frames = frames .* frames;

% I still have a 4-value oversampling here: go back to means and stds.
avgs = zeros(size(frames, 1), numFrames);
stds = avgs;
for k = 1:numFrames
  avgs(:, k) = mean(frames(...
      :, k*oversample-oversample+1:min([k*oversample, size(frames, 2)])), 2);
  stds(:, k) = std(frames(...
      :, k*oversample-oversample+1:min([k*oversample, size(frames, 2)])), 1, 2);
end

stds = stds ./ avgs; % normalize standard deviation by the mean value.

% calculate the banks
banks = zeros(size(avgs, 2), size(filterbank, 2));
voic = banks;
for k = 1:size(filterbank, 2)
  banks(:, k) = sum(avgs .* (filterbank(:, k) * ones(1, size(avgs, 2))))';
  voic(:, k) = sum(stds .* (filterbank(:, k) * ones(1, size(avgs, 2))))';
end

% and back to amplitudes:
R = sqrt(banks);

if (nargout > 1)
  voicedness = -voic;
end   