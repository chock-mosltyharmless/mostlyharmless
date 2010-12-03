function R = waveletEncode(A, scale, xScale, yScale, minValue)
% The larger scale is, the more it is compressed. scale must be larger than 0,
% xScale and yScale may be zero for uniform quantization
%
% This is not realistic, because well... The output might not be integral.
% I might have to come up with something different.
R = A;

xMatrix = ones(16, 16) + xScale * repmat((0:15)./16, 16, 1);
yMatrix = ones(16, 16) + yScale * repmat((0:15)'./16, 1, 16);

for k = 1:16:size(A, 2)
  R(:, k:k+15) = encodeBlock(A(:, k:k+15));
  
  % VQ:
  R(:, k:k+15) = R(:, k:k+15) ./ xMatrix ./ yMatrix ./ scale;
  R(find(abs(R) < minValue)) = zeros(size(find(abs(R) < minValue)));
  R(:, k:k+15) = xMatrix .* yMatrix .* scale .* floor(R(:, k:k+15) + 0.5);
end
 