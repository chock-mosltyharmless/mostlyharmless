% Loads the file from filename and saves it in orig.
% The spectrogram is generated and stored in spec.

FFTLength = 1024;
%frameStep = FFTLength / 2;
frameStep = 512;

barkBorders = [20, 100, 200, 300, 400, 510, 630, 770, 920, 1080, ...
               1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, ...
               5300, 6400, 7700, 9500, 12000, 15500];


% Load file
orig = wavread(filename);
orig = orig(1:end-50, 1);
%orig = randn(22050, 1) ./ 5;

% Create the FFT filter coefficients:
FFTCoefs = FFTFilterCoefs(barkBorders, FFTLength);

% create spectrogram
spec = spgram(orig, FFTLength, frameStep);

% filter:
%rec = zeros(length(orig), length(barkBorders)-1);
%for k = 1:length(barkBorders)-1
  %sp = spec .* repmat(FFTCoefs(:, k), 1, size(spec, 2));
  %recs = reconstruct(sp);
  %rec(1:length(recs), k) = recs;
%end
