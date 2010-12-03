function R = reconstruct(spec)
  
  FFTLength = size(spec, 1);
  frameStep = FFTLength/2;
  
  ispec = ifft(spec);
  % apply hamming window
  %win = hanning(FFTLength);
  win = [1:FFTLength/2, FFTLength/2:-1:1]' ./ FFTLength/2;
  win = sqrt(win);
  % sub mean? no.
  %frames = frames - repmat(mean(frames), size(frames, 1), 1);
  ispec = ispec .* (repmat(win, 1, size(ispec, 2)));
  
  R = zeros(frameStep*(size(spec, 2)+1), 1);
  
  for k = 0:size(spec, 2)-1
    R(k*frameStep+1:k*frameStep+FFTLength) = ...
      R(k*frameStep+1:k*frameStep+FFTLength) + ispec(:, k+1);
  end
  
  R = real(R);