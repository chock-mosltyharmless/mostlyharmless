function R = feedbackComb(A, alpha, K)
% Linear ramp up of alpha.
  
  R = [zeros(K, 1); A];
  
  for t = 1:length(A)
    alp = log(alpha)*(t-1)/length(A);
    R(t+K) = A(t) - (1-exp(alp))*R(t);  
  end
  
  R = R(K+1:end);
  
  %Readjust
  for t = 1:length(A)
    alp = log(alpha)*(t-1)/length(A);
    R(t) = R(t) / (2-exp(alp));
  end