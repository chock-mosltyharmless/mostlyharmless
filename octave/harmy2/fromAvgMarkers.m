function R = fromAvgMarkers(avgMarkers, stepSize)
% R = fromAvgMarkers(avgMarkers, stepSize)
%
% Creates markers from avgMarker positions.
% The last generated marker is maximal length(avgMarkers)*stepSize

maxMarker = length(avgMarkers)*stepSize;
R = [];

% repeat last marker:
avgMarkers(length(avgMarkers)+1) = avgMarkers(length(avgMarkers));

pos = avgMarkers(1);
while(pos < maxMarker)
  R = [R; pos];
  ind = floor(pos/stepSize) + 1;
  % linear interpolation:
  delta = pos/stepSize + 1 - ind;
  pos = pos + delta * avgMarkers(ind+1) + (1-delta) * avgMarkers(ind);
  %pos = pos + avgMarkers(ind);
end 