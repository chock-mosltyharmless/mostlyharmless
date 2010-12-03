function R = waveletDecode(A)
R = A;

for k = 1:16:size(A, 2)
  R(:, k:k+15) = decodeBlock(A(:, k:k+15));
end
 