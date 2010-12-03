function R = applyIIRBank(A, energies, stepSize, frequencies)
% R = applyIIRBank(A, energies, stepSize, frequencies)
%
% applies an IIR filterbank. debug version for testing only

% at first, upsample energies.
energies = resample(energies, stepSize, 1);
ener = max(zeros(size(energies)), energies);

% extend up to length of A.
energies = zeros(length(A), size(ener, 2));
energies(1:size(ener, 1), :) = ener;

% Butterworth filterband. At first really simple...
% I have to exchange these with the bark filterbanks.
%b = zeros(24, 3);
%a = zeros(24, 3);
%frequencies = (1.25 .^ (0:24)') * 75;
for k = 1:length(frequencies)-2
  [b(k, :), a(k, :)] = butter(2, [frequencies(k)./22050, frequencies(k+2)./22050]);
end

% calculate the reconstructed stuff.
bands = zeros(size(A));
%plot(filter(b(1, :), a(1, :), A(1:3000)))
for k = 1:length(frequencies)-2
  bands(:, k) = filter(b(k, :), a(k, :), filter(b(k, :), a(k, :), filter(b(k, :), a(k, :), A))) .* energies(:, k);
  %bands(:, k) = filter(b(k, :), a(k, :), A) .* energies(:, k);
end

% normalize to max amplitude = 1 (should be done differently...)
%reconstructed = reconstructed(imarkers(1):end);
%reconstructed = reconstructed ./ max(abs(reconstructed));

R = sum(bands, 2);
