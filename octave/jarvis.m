function R = jarvis(A, checkerboard, oldold)

R = A;
adder = zeros(size(A, 1)+2, size(A, 2)+4);
matr = [0 0 0 7 5; 3 5 7 5 3; 1 3 5 3 1]./48;

for y = 1:size(R, 1);
  for x = 1:size(R, 2);
    oldpixel = A(y, x) + adder(y, x+2);
    oldoldpixel = oldpixel;
    
    % checkerboard
    if (nargin > 1)
      if (nargin < 3)
        if (nargin > 1 && mod(x+y,2) == 0) oldpixel = oldpixel + 1/6; end
        if (nargin > 1 && mod(x+y,2) == 1) oldpixel = oldpixel - 1/6; end
      end
      if (nargin > 2)
        oldpixel = oldpixel + (rand() - 0.5) * 0.25;
      end
    end
    
    if (oldpixel < 0.5)
      newpixel = 0;
    else
      newpixel = 1;
    end
    
    R(y, x) = newpixel;

    quantError = oldpixel - newpixel;

    adder(y:y+2, x:x+4) = adder(y:y+2, x:x+4) + quantError .* matr;
  end
end