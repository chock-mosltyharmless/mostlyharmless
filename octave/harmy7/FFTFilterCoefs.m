function R = FFTFilterCoefs(barkBorders, FFTLength)
  
  R = zeros(FFTLength, length(barkBorders)-1);
  
  for k = 1:length(barkBorders)-1
    startPos = barkBorders(k) / 44100 * FFTLength + 1;
    endPos = barkBorders(k+1) / 44100 * FFTLength + 1;
    
    R(ceil(startPos):floor(endPos), k) = ...
      ones(length(ceil(startPos):floor(endPos)), 1);
    R(floor(startPos), k) = ...
      floor(startPos) + 1 - startPos;
    R(floor(endPos), k) = ...
      endPos - floor(endPos);
  end
  
  % Bottom-top
  R(FFTLength/2+2:FFTLength, :) = flipud(R(2:FFTLength/2, :));
  