function R = pulsetrain2(markers, signalSize)
% R = pulsetrain(markers, signalSize)
%
% Create a pulse train from the position in markers.
% The size of the generated timeseries is given in signalSize.

R = zeros(signalSize);
for k = 1:length(markers)-1
  pos = markers(k);  
  %ipos = floor(markers(k)+0.5);  
  %startP = max(1, ipos-100);
  %endP = min(max(signalSize), ipos+100);
  %energy = sqrt(markers(k+1)-markers(k));  
  %R(startP:endP) = R(startP:endP) - sqrt(markers(k+1)-markers(k))/(endP-startP+1) + energy * sinc(0.75*(startP-markers(k):endP-markers(k)))';
  ipos = floor(markers(k));
  dpos = markers(k) - ipos;
  energy = sqrt(markers(k+1)-markers(k)); % I do not want sqrt!
  energy = 14;
  R(ipos) = (1-dpos) * energy;
  R(ipos+1) = dpos * energy;
end

%DC blocker:
oldy = 0;
oldx = 0;
for k = 1:length(R)
  oldy = R(k) - oldx + 0.98 * oldy;
  oldx = R(k);
  R(k) = oldy;
end
