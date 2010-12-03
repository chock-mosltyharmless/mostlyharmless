function R = applyBank(A, energies, FFTLength, filterbank)
% R = applyBank(A, energies, FFTLength, filterbank)
%
% Applies a filter bank to A

% Overlapping whatever
FFTstep = FFTLength/2;
ind = ones(FFTLength, 1) * (0:FFTstep:length(A)-FFTLength);
ind = ind + (1:FFTLength)' * ones(1, size(ind, 2));
frames = A(ind);

% apply FFT
fftFrames = fft(frames);

% calculate the window
win = bartlett(FFTLength); % triangular window

for k = 1:size(fftFrames, 2)
  % create the modifier that is used to change the fft.
  modifier = sum(repmat(energies(k, :), 2048, 1) .* filterbank, 2);
  fftFrames(:, k) = fftFrames(:, k) .* modifier;
  % and do ifft:
  fftFrames(:, k) = real(ifft(fftFrames(:, k)));
  % apply window
  fftFrames(:, k) = fftFrames(:, k) .* win;  
end

% calculate result
R = zeros(size(A));
for k = 0:size(fftFrames, 2)-1
  R(k*FFTstep+1:k*FFTstep+FFTLength) = R(k*FFTstep+1:k*FFTstep+FFTLength) + ...
                                       fftFrames(:, k+1);
end



