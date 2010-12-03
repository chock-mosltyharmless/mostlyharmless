function R = waveform(len, reduction)
  
  R = zeros(len, 1);
  strength = 1;
  for k = 1:floor(len/2) - 1
    R = R + strength * sin((0:len-1)' / len * 2 * pi * k);
    strength = strength * reduction;
  end
  
  %normalize
  R = R ./ sqrt(sum(R.*R));