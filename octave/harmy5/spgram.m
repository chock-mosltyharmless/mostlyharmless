function R = spgram(A, frameStep)
  
FFTLength = 2*frameStep;
  
 % I calculate the length of the original working stuff...
numFrames = floor((length(A)-FFTLength)/frameStep)+1;

% I am oversampling this oversample times.
ind = ones(FFTLength, 1) * (0:frameStep:length(A)-FFTLength);
ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
frames = A(ind);

% apply hamming window
%win = hanning(FFTLength);
win = [1:FFTLength/2, FFTLength/2:-1:1]' ./ FFTLength/2;
win = sqrt(win);
% sub mean? no.
%frames = frames - repmat(mean(frames), size(frames, 1), 1);
frames = frames .* (repmat(win, 1, size(frames, 2)));

% FFT
R = fft(frames);
