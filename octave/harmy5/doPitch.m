% Well.. just to get orig and defaults basically :-)
numMaxima = 4; % outdataed.
rescaler = 16;
viterbiJumpCost = 0.1; %hmm?

%if (false)
doFilter

logFrs = log2(frameStep)+1:13;

% Run framedCeps for different frameLengths
for logFr = logFrs
  fr = floor(2^logFr + 0.5);  
  Rceps{logFr} = framedCeps(orig, fr, frameStep);
end

% Get maximum:
RcepsAll = Rceps{logFrs(end)};
numFrames = size(RcepsAll, 1);

for logFr = logFrs(end-1:-1:1)
  adder = (2^logFrs(end)-2^logFr);
  curLen = size(Rceps{logFr}, 2);
  win = hanning(curLen).*hanning(curLen);
  win = repmat(win', size(RcepsAll, 1), 1);
  invWin = 1-win;
  %RcepsAll(:, adder+1:adder+curLen) = max(RcepsAll(:, adder+1:adder+curLen), Rceps{logFr});
  %RcepsAll(:, adder+1:adder+curLen) = (RcepsAll(:, adder+1:adder+curLen) + Rceps{logFr})/2;
  RcepsAll(:, adder+1:adder+curLen) = invWin .* RcepsAll(:, adder+1:adder+curLen) + win .* Rceps{logFr};
end

% Go through all the frames and get maximum peak quefrency
values = zeros(size(RcepsAll, 1), 1);
rizens = zeros(size(RcepsAll, 1), 1);
middle = ceil(size(RcepsAll, 2)/2);
for frame = 1:size(RcepsAll, 1);
  value = 0;
  current = inf;
  while(value < middle && current >= RcepsAll(frame, value+middle))    
    current = RcepsAll(frame, value+middle);
    RcepsAll(frame, value+middle) = 0;
    value = value + 1;    
  end
  
  % max is from value to end:
  [rize, tmp] = max(RcepsAll(frame, value+middle:end));
  value = value + tmp;
  values(frame) = value;
  rizens(frame) = rize;
end

% find all local maxima:
maxima.pos = ones(numFrames, numMaxima);
maxima.val = -1*ones(numFrames, numMaxima);

for k = 1:numFrames
  left = RcepsAll(k, middle:end-2);
  right = RcepsAll(k, middle+2:end);
  all = RcepsAll(k, middle+1:end-1);
  curMax = find((all > right) & (all > left)) + 1;
  curVal = RcepsAll(k, curMax + middle);
  % sort maxima:
  [curVal, I] = sort(curVal, 'descend');
  curMax = curMax(I);
  % get only to numMaxima
  if (length(curMax) > numMaxima)
    curMax = curMax(1:numMaxima);
    curVal = curVal(1:numMaxima);
  end
  maxima.pos(k, 1:length(curMax)) = curMax;
  maxima.val(k, 1:length(curMax)) = curVal;
end

%end

% resale the RcepsAll matrix into viterbimatrix
rescale.matrix = -2 * ones(numFrames, floor(log2(middle)*rescaler + 1.5));

for k = 1:size(RcepsAll, 2)-middle
  pos = floor(log2(k)*rescaler + 1.5);
  rescale.matrix(:, pos) = max(rescale.matrix(:, pos), RcepsAll(:, middle+k));
end
for k = size(rescale.matrix, 2)-1:-1:1
  rescale.pos(k) = 2.^((k-1) / rescaler);
  if (rescale.matrix(1, k) < -1)
    rescale.matrix(:, k) = rescale.matrix(:, k+1);
  end
end

% Viterbi:
%init
if (false)
viterbi.val = maxima.val;
viterbi.previous = zeros(size(maxima.pos));
for k = 2:numFrames
  jumps = abs(repmat(log(maxima.pos(k, :)), numMaxima, 1) - ...
              repmat(log(maxima.pos(k-1, :)'), 1, numMaxima));
  jumpCost = -jumps * viterbiJumpCost;
  curVals = repmat(maxima.val(k, :), numMaxima, 1) + ...
            repmat(viterbi.val(k-1, :)', 1, numMaxima) + jumpCost;
  [viterbi.val(k, :), viterbi.previous(k, :)] = max(curVals);
end
% calculate the final path
[tmp, best] = max(viterbi.val(end, :));
pitch = maxima.pos(end, best(1));
for k = numFrames:-1:2
  best = [viterbi.previous(k, best(1)); best];
  pitch = [maxima.pos(k-1, best(1)); pitch];
end
end

numMaxima = size(rescale.matrix, 2);
viterbi.val = rescale.matrix;
viterbi.previous = zeros(size(rescale.matrix));
for k = 2:numFrames
  %jumps = abs(repmat(1:numMaxima, numMaxima, 1) - ...
  %            repmat((1:numMaxima)', 1, numMaxima));
  jumps = abs(sqrt(repmat(1:numMaxima, numMaxima, 1)) - ...
              sqrt(repmat((1:numMaxima)', 1, numMaxima)));
  jumpCost = -jumps * viterbiJumpCost;
  curVals = repmat(rescale.matrix(k, :), numMaxima, 1) + ...
            repmat(viterbi.val(k-1, :)', 1, numMaxima) + jumpCost;
  [viterbi.val(k, :), viterbi.previous(k, :)] = max(curVals);
end
% calculate the final path
[tmp, best] = max(viterbi.val(end, :));
pitch = rescale.pos(best(1));
for k = numFrames:-1:2
  best = [viterbi.previous(k, best(1)); best];
  pitch = [rescale.pos(best(1)); pitch];
end
