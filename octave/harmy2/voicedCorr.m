function R = voicedCorr(A, numFrames, markers, maxVoicedLength)
% R = voicedCorr(A, numFrames, markers, maxVoicedLength)
%
% Takes away the unvoiced parts of A using a cross-correlation method.
% numFrames are the number of frames that are used for correlation checking.
% markers are the pitch marker positions in samples
% maxVoicedLength is the longest wavelength that is still considered voiced.
% The first and last some samples are always considered unvoiced.

R = zeros(size(A));

A = [zeros(maxVoicedLength*numFrames, 1); A; zeros(maxVoicedLength*numFrames, 1)];

for k = 1:length(markers)-1
  pitchLen = markers(k+1) - markers(k);
  if (pitchLen <= maxVoicedLength)
    startP = floor(markers(k)) + 1;
    endP = floor(markers(k+1));
    pitchLenI = endP - startP + 1;
    
    % Calculate the parts in A to correlate
    Abuf = zeros(pitchLenI, 2*numFrames+1);
    for k = -numFrames:numFrames
      Abuf(:, k+numFrames + 1) = A(maxVoicedLength*numFrames + ...
          [startP-k*pitchLenI:endP-k*pitchLenI]);
    end   
    
    R(startP:endP) = sum(Abuf, 2)./(2*numFrames+1);
  end
end