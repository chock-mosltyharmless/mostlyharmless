% Well.. just to get orig and defaults basically :-)
rescaler = 64;
%viterbiJumpCost = [0.0125, 0.025, 0.05, 0.1, 0.2]; %hmm?
viterbiJumpCost = [0.0125]; %hmm?

disp("Init:");
tic
%if (false)
doFilter
toc

disp("Calculating cepstra")
tic
logFrs = floor(log2(frameStep))+1:1:13; % oh, no...

% Run framedCeps for different frameLengths
for logFr = logFrs
  fr = floor(2^logFr + 0.5);  
  Rceps{logFr} = framedCeps(orig, fr, frameStep);
end

toc
disp("Combining cepstra")
tic

% overlay receps:
RcepsAll = Rceps{logFrs(end)};
RcepsAll = RcepsAll(:, 1:size(RcepsAll, 2)./4);
numFrames = size(RcepsAll, 1);

for logFr = logFrs(end-1:-1:1)
  curLen = size(Rceps{logFr}, 2);
  halfLen = size(Rceps{logFr}, 2)./2;
  win = hanning(curLen).*hanning(curLen);
  win = repmat(win(halfLen+1:curLen)', size(RcepsAll, 1), 1);
  invWin = 1-win;
  %RcepsAll(:, adder+1:adder+curLen) = max(RcepsAll(:, adder+1:adder+curLen), Rceps{logFr});
  %RcepsAll(:, adder+1:adder+curLen) = (RcepsAll(:, adder+1:adder+curLen) + Rceps{logFr})/2;
  RcepsAll(:, 1:halfLen) = invWin .* RcepsAll(:, 1:halfLen) + win .* Rceps{logFr}(:, 1:halfLen);
end
RcepsCopy = RcepsAll;
toc

disp("Removing DC in cepstrum")
tic

% Go through all the frames and get maximum peak quefrency
numFrames = size(RcepsAll, 1);
numLags = size(RcepsAll, 2);
values = zeros(numFrames, 1);
rizens = zeros(numFrames, 1);

for frame = 1:size(RcepsAll, 1);
  value = 1;
  current = inf;
  while(value < numLags && current >= RcepsAll(frame, value))
    current = RcepsAll(frame, value);
    RcepsAll(frame, value) = 0;
    value = value + 1;    
  end
  
  % max is from value to end:
  [rize, tmp] = max(RcepsAll(frame, value:end));
  value = value + tmp - 1;
  values(frame) = value;
  rizens(frame) = rize;
end

toc

disp("Log. rescaling of cepstra:")
tic

% resale the RcepsAll matrix into viterbimatrix
rescale.matrix = -2 * ones(numFrames, floor(log2(numLags)*rescaler + 1.5));

for k = 1:size(RcepsAll, 2)
  pos = floor(log2(k)*rescaler + 1.5);
  rescale.matrix(:, pos) = max(rescale.matrix(:, pos), RcepsAll(:, k));
end
for k = size(rescale.matrix, 2)-1:-1:1
  rescale.pos(k) = 2.^((k-1) / rescaler);
  if (rescale.matrix(1, k) < -1)
    rescale.matrix(:, k) = rescale.matrix(:, k+1);
  end
end

toc

disp("Viterbi:")
tic

% Viterbi:
pitch = zeros(numFrames, length(viterbiJumpCost));
for jc = 1:length(viterbiJumpCost)
  numMaxima = size(rescale.matrix, 2);  
  viterbi{jc}.val = rescale.matrix;
  viterbi{jc}.previous = zeros(size(rescale.matrix));
  for k = 2:numFrames
    jumps = abs(repmat(1:numMaxima, numMaxima, 1) - ...
                repmat((1:numMaxima)', 1, numMaxima));
    %jumps = abs(sqrt(repmat(1:numMaxima, numMaxima, 1)) - ...
    %            sqrt(repmat((1:numMaxima)', 1, numMaxima)));
    jumpCost = -jumps * viterbiJumpCost(jc);
    curVals = repmat(rescale.matrix(k, :), numMaxima, 1) + ...
              repmat(viterbi{jc}.val(k-1, :)', 1, numMaxima) + jumpCost;
    [viterbi{jc}.val(k, :), viterbi{jc}.previous(k, :)] = max(curVals);
  end
  % calculate the final path
  [tmp, best] = max(viterbi{jc}.val(end, :));
  pitch(end, jc) = rescale.pos(best(1));
  for k = numFrames:-1:2
    best = [viterbi{jc}.previous(k, best(1)); best];
    pitch(k-1, jc) = rescale.pos(best(1));
  end
end

noencode

toc
