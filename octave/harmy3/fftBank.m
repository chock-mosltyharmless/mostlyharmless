function [voiced, unvoiced] = fftBank(A, FFTLength, filterbank, voicFilename)
% [voiced, unvoiced] = fftBank(A, FFTLength, filterbank, voicFilename)
%
% Calculate the spectrogramm of A and calculate the bank energies using filterbank
% (which is calculated w.g. by barkFilterCoefs).
% voicFilename is the name of a .tga file that... whatever. If it's left out,
% no voicedness is done, and only voiced is returend (needs implementation!)

voic = [];

% normalize the filterbank.
% This destroys the reconstruction!
for k = 1:size(filterbank, 2)
  filterbank(:, k) = filterbank(:, k) ./ sum(filterbank(:, k));
end

% I calculate the length of the original working stuff...
numFrames = floor((length(A)-FFTLength)/FFTLength*2)+1;

% I am oversampling this oversample times.
FFTstep = FFTLength/4;
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

%load voicedness .tga ...
if (nargin > 3)
  voicedness = tgaRead(voicFilename);
  voicedness = voicedness(:, :, 1);
  voicedness = [voicedness; voicedness(end, :);flipud(voicedness(2:end, :))];
  %imsc(voicedness)
else
  voicedness = ones(size(frames));
end   

voicFrames = voicedness .* frames;
unvoicFrames = (1-voicedness) .* frames;

% calculate the banks
vbanks = zeros(size(frames, 2), size(filterbank, 2));
uvbanks = vbanks;
for k = 1:size(filterbank, 2)
  vbanks(:, k) = sum(voicFrames .* (filterbank(:, k) * ones(1, size(frames, 2))))';
  uvbanks(:, k) = sum(unvoicFrames .* (filterbank(:, k) * ones(1, size(frames, 2))))';
end

% and back to amplitudes:
voiced = sqrt(vbanks);
unvoiced = sqrt(uvbanks);
