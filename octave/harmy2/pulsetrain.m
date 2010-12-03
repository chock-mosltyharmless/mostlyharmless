function R = pulsetrain(markers, signalSize)
% R = pulsetrain(markers, signalSize)
%
% Create a pulse train from the position in markers.
% The size of the generated timeseries is given in signalSize.

R = zeros(signalSize);
for k = 1:length(markers)
  pos = markers(k);  
  ipos = floor(markers(k)+0.5);
  %dpos = pos - ipos;
  startP = max(1, ipos-100);
  endP = min(max(signalSize), ipos+100);
  R(startP:endP) = R(startP:endP) + sinc(startP-markers(k):endP-markers(k))';
  %R(ipos-100:ipos+100) = sinc(dpos-100:dpos+100);
end
