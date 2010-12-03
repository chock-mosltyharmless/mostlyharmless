function R = klatttrain(markers, signalSize, parameter)
% R = klatttrain(markers, signalSize, parameter)
%
% Create a klatt train from the position in markers.
% The size of the generated timeseries is given in signalSize.

R = zeros(signalSize);
for k = 1:length(markers)-1
  pos = markers(k);  
  ipos = floor(markers(k));
  %dpos = pos - ipos;
  startP = max(1, ipos);
  ipos2 = ceil(markers(k+1));
  endP = min(max(signalSize), ipos2);
  energy = sqrt(markers(k+1)-markers(k));
  R(startP:endP) = R(startP:endP) + energy * klatt((startP-markers(k):endP-markers(k))', ...
      markers(k+1)-markers(k), parameter);
  %R(ipos-100:ipos+100) = sinc(dpos-100:dpos+100);
end
