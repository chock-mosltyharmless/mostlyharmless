function R = framedAutocorr(A, frameSize, maxStep, frameStep, corrStep)
  % framesize must be even
  
  startP = 1;
  endP = length(A)-frameSize-maxStep+1;
  framePositions = startP:frameStep:endP;
  numFrames = length(framePositions);
  stepPositions = 0:corrStep:maxStep;
  numSteps = length(stepPositions);
  
  R = zeros(numFrames, numSteps);
  
  win = hamming(frameSize);
  
  for fr = 1:numFrames
    k = framePositions(fr);
    firstFrame = A(k:k+frameSize-1) .* win;
    firstFrame = firstFrame ./ sqrt(sum(firstFrame .* firstFrame));
    for st = 1:numSteps
      l = stepPositions(st);
      secondFrame = A(k+l:k+l+frameSize-1) .* win;
      secondFrameE = sqrt(sum(secondFrame .* secondFrame));
      R(fr, st) = sum(firstFrame .* secondFrame ./ secondFrameE);
    end
  end