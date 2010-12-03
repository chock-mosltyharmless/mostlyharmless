function block = decodeBlock(blockWavelet)
  
% In time dimension:
for y = 1:16  
  stuffer = ones(1, 16);
  stuffer = stuffer ./ sqrt(sum(stuffer.*stuffer));
  xpos = 2;
  st(1, :) = stuffer;
  for numStuff = [1, 2, 4, 8]
    for thingie = 0:numStuff-1      
      startpos = thingie*(16/numStuff)+1;
      endpos = (thingie+1)*(16/numStuff);
      stuffer = (-8/numStuff+0.5:8/numStuff-0.5);
      stuffer = stuffer ./ sqrt(sum(stuffer.*stuffer));
      st(xpos, startpos:endpos) = stuffer;
      xpos = xpos+1;      
    end    
  end
end

block = st' * blockWavelet * st;