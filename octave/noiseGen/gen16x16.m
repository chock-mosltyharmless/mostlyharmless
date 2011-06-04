A = rand(16,16,4) - 0.5;
A = A ./ repmat(sqrt(sum(A.*A, 3)), [1, 1, 4]);
writeDDS('16x16a.dds', 0.5*A+0.5);

A = rand(16,16,4) - 0.5;
A = A ./ repmat(sqrt(sum(A.*A, 3)), [1, 1, 4]);
writeDDS('16x16b.dds', 0.5*A+0.5);