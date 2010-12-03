function imsc(A)
% imsc(A)
%
% works just like imagesc in matlab.

A = A - min(min(A)) + 1;
A = A ./ max(max(A)) .* length(colormap());
h = axes();
image(A);
set(h, 'XLimMode', 'manual');
set(h, 'XLim', [0.5, size(A, 2)+0.5]);
set(h, 'YLimMode', 'manual');
set(h, 'YLim', [0.5, size(A, 1)+0.5]);