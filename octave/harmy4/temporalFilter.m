function R = temporalFilter(A)
% filter small peaks along first dimension

for k = 1:size(A, 2)
  oldValue = A(1, k);
  oldPosition = 1;
  for t = 2:size(A, 1)
    if (A(t, k) == oldValue)
      % There was only a slight change, flatten everything inbetween:
      A(oldPosition:t, k) = oldValue * ones(t-oldPosition+1, 1);
      oldPosition = t;
    end
    
    if (abs(A(t, k) - oldValue) > 1)
      % There was a relevant change, update position
      oldPosition = t;
      oldValue = A(t, k);
    end
  end
end

R = A;
  