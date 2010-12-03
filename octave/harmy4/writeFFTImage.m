function R = writeFFTImage(A, FFTLength)
  
% preemphasise A
%A = filter([-1, 1], 1, A);
  
FFTstep = FFTLength/4;
ind = ones(FFTLength, 1) * (0:FFTstep:length(A)-FFTLength);
ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
frames = A(ind);

size(frames)

% apply hamming window
% TODO: I might consider using some chebyshef window here, maybe with
% longer FFT length.
win = hanning(FFTLength);
%win = nuttallwin(FFTLength)';
% sub mean?
frames = frames - repmat(mean(frames), size(frames, 1), 1);
frames = frames .* (repmat(win, 1, size(frames, 2)));

% calculate fft
frames = log(abs(fft(frames)) + 1.0e-20);

%frames = log(abs(specgram(A, FFTLength, 44100, 1024, 1024-FFTstep)));

% scale frames to 1:1024 (int for jet)
frames = frames - min(min(frames));
frames = frames ./ max(max(frames));
frames = 1-frames; % for jet?
frames = frames * 1023 + 1;
frames = int32(frames);
colmap = jet(1024);

frames = frames(1:FFTLength/2, :);

img = zeros(size(frames, 1), size(frames, 2), 3);
img(:, :, 1) = colmap(frames(:, :), 1);
img(:, :, 2) = colmap(frames(:, :), 2);
img(:, :, 3) = colmap(frames(:, :), 3);
%for x = 1:size(frames, 1);
%  for y = 1:size(frames, 2);
%    img(:, :, 1) = colmap(frames(x, y), 1);
%    img(:, :, 2) = colmap(frames(x, y), 2);
%    img(:, :, 3) = colmap(frames(x, y), 3);
%  end 
%end

tgaWrite('spectrogram.tga', img);