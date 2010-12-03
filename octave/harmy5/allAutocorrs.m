function R = allAutocorrs(A, barkBorders)
  
  numFilters = length(barkBorders)-1;
  R = [];
  
  for k = 1:numFilters
    maxSt = ceil(44100/barkBorders(k));
    minSt = ceil(maxSt ./ 10);
    frameSize = maxSt * 3;
    frameStep = ceil(frameSize / 4);
    atc = framedAutocorr(A, frameSize, maxSt, frameStep, minSt);
    R{k} = atc;
  end