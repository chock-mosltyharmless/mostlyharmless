function R = spgram(A, FFTLength, frameStep)

A = [zeros(floor((FFTLength-frameStep)./2), 1); A; zeros(ceil((FFTLength-frameStep)./2), 1)];
  
 % I calculate the length of the original working stuff...
numFrames = floor((length(A)-FFTLength)/frameStep)+1;

% I am oversampling this oversample times.
ind = ones(FFTLength, 1) * (0:frameStep:length(A)-FFTLength);
ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
frames = A(ind);

% apply hamming window
%win = hanning(FFTLength);
%win = [1:FFTLength/2, FFTLength/2:-1:1]' ./ FFTLength .* 2;
%win = min(ones(FFTLength, 1), [1:FFTLength/2, FFTLength/2:-1:1]' ./ FFTLength .* 4);
%win = sqrt(win);
%win = hann(FFTLength);

% My new window adds to 1...
win = hanning(FFTLength);
win(FFTLength/2+1:end) = 1 - win(1:FFTLength/2);
win = sqrt(win);

% sub mean? no.
%frames = frames - repmat(mean(frames), size(frames, 1), 1);
frames = frames .* (repmat(win, 1, size(frames, 2)));

% FFT
R = fft(frames);
