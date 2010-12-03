function R = applyIIRBank(A, energies, stepSize, frequencies)
% R = applyIIRBank(A, energies, stepSize, frequencies)
%
% applies an IIR filterbank. debug version for testing only

% at first, upsample energies.
%energies = resample(energies, stepSize, 1);
%Now that was some evil resample. I'd rather interpolate?
energ2 = [];
for k = 1:size(energies, 2)
  energ2(:, k) = interp1(1:stepSize:stepSize*size(energies, 1), energies(:, k)', ...
      1:stepSize*size(energies, 1)-stepSize+1, 'linear')';
end
ener = max(zeros(size(energ2)), energ2);

% extend up to length of A.
energies = zeros(length(A), size(ener, 2));
energies(1:size(ener, 1), :) = ener;

% Butterworth filterband. At first really simple...
% I have to exchange these with the bark filterbanks.
%b = zeros(24, 3);
%a = zeros(24, 3);
%frequencies = (1.25 .^ (0:24)') * 75;
for k = 1:length(frequencies)-1
  if (k > 1)
    freq1 = frequencies(k) - 0.1 * (frequencies(k+1) - frequencies(k));
  else
    freq1 = frequencies(k);
  end
  freq2 = frequencies(k+1) + 0.1 * (frequencies(k+1) - frequencies(k));
  [b(k, :), a(k, :)] = butter(3, [freq1./22050, freq2./22050]);
end

% calculate the reconstructed stuff.
bands = zeros(size(A));
%plot(filter(b(1, :), a(1, :), A(1:3000)))
for k = 1:length(frequencies)-1
  bands(:, k) = filter(b(k, :), a(k, :), filter(b(k, :), a(k, :), A)) .* energies(:, k);
  %bands(:, k) = filter(b(k, :), a(k, :), A) .* energies(:, k);
end

% normalize to max amplitude = 1 (should be done differently...)
%reconstructed = reconstructed(imarkers(1):end);
%reconstructed = reconstructed ./ max(abs(reconstructed));

R = sum(bands, 2);
