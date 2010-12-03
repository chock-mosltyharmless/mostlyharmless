function R = floydSteinberg(A, checkerboard, oldold)
  
R = A;
A(end+1, end+1) = 0; % whatever...

checker = [1 49 13 61 4 52 16 64; ...
           33 17 45 29 36 20 48 32; ...
           9  57  5 53 12 60  8 56; ...
           41 25 37 21 44 28 40 24; ...
           3  51 15 63  2 50 14 62; ...
           35 19 47 31 34 18 46 30; ...
           11 59  7 55 10 58  6 54; ...
           43 27 39 23 42 26 38 22] ./ 65;
           
checker = [3 7 4; ...
           6 1 9; ...
           2 8 5] ./ 10;

for y = 1:size(R, 1);
  for x = 1:size(R, 2);
    oldpixel = A(y, x);
    oldoldpixel = oldpixel;
    
    % checkerboard
    if (nargin > 1)      
      if (nargin < 3)
        if (mod(x+y,2) == 0) oldpixel = oldpixel + 1/6; end
        if (mod(x+y,2) == 1) oldpixel = oldpixel - 1/6; end
      end
      if (nargin > 2)
        %oldpixel = oldpixel + checker(mod(x, size(checker, 1))+1, mod(y, size(checker, 2))+1) - 0.5;
        oldpixel = oldpixel + (rand() - 0.5) * 0.25;
      end
    end
    %if (nargin > 1 && mod(x,2) == 0 && mod(y, 2) == 0) oldpixel = oldpixel - 3/10; end
    %if (nargin > 1 && mod(x,2) == 0 && mod(y, 2) == 1) oldpixel = oldpixel + 1/10; end
    %if (nargin > 1 && mod(x,2) == 1 && mod(y, 2) == 0) oldpixel = oldpixel + 3/10; end
    %if (nargin > 1 && mod(x,2) == 1 && mod(y, 2) == 1) oldpixel = oldpixel - 1/10; end
    
    if (oldpixel < 0.5)
      newpixel = 0;
    else
      newpixel = 1;
    end
    
    R(y, x) = newpixel;
        
    quantError = oldpixel - newpixel;
    A(y+1, x) = A(y+1, x) + 7 * quantError/16;
    A(y+1, x+1) = A(y+1, x+1) + 1 * quantError/16;
    A(y, x+1) = A(y, x+1) + 5 * quantError/16;
    if (x > 1)
      A(y+1, x-1) = A(y+1, x-1) + 3 * quantError/16;
    end
  end
end