%filename = 'HUMAN_ACAPELLA_SHO_DO_WOP_DO_AHH_01';
filename = 'HUMAN_ELEMENT_MALE_HALLEUJAH_LONG_01';

% Load the sound and point process.
A = wavread([filename, '.wav']);
A = A(1:end-30, 1);
markers = loadPointProcess([filename, '.PointProcess']);
imarkers = floor(markers+0.5);

% Butterworth filterband. At first really simple...
b = zeros(24, 5);
a = zeros(24, 5);
frequencies = (1.25 .^ (0:24)') * 75;
for k = 1:24  
  [b(k, :), a(k, :)] = butter(2, [frequencies(k)./22050, frequencies(k+1)./22050]);
end

% Generate the bandpath filtered sounds and their envelopes E
% I have to make a function of this.
B = zeros(size(A, 1), 1);
E = zeros(size(A, 1), 24);
bLo = fir1(4096, 16/22050);
for k = 1:24
  B = filter(b(k, :), a(k, :), A);
  %E(:, k) = abs(hilbert(B(:, k)));
  % Hilbert substitute (remove for final)
  E(:, k) = abs(B);
  % 16 Hz lowpass filter of E (brick wall FIR)
  E(:, k) = fftfilt(bLo, E(:, k));
end

% Recreate the sound using an unvoiced exec
%excitation = rand(size(A));

% I will rather create a voiced exec from the markers:
excitation = zeros(size(A));
for k = 1:length(markers)
  pos = markers(k);  
  ipos = imarkers(k);
  dpos = pos - ipos;
  excitation(ipos-100:ipos+100) = sinc(-100+dpos:100+dpos);
end

% now get the excitation envelopes:
B = zeros(size(A, 1), 1);
exE = zeros(size(A, 1), 24);
bLo = fir1(4096, 16/22050);
for k = 1:24
  B = filter(b(k, :), a(k, :), excitation);
  %E(:, k) = abs(hilbert(B(:, k)));
  % Hilbert substitute (remove for final)
  exE(:, k) = abs(B);
  % 16 Hz lowpass filter of E (brick wall FIR)
  exE(:, k) = fftfilt(bLo, exE(:, k));
end

% calculate the normalized E.
% I have to do this in a much more clever way.
% 1) what to do if exE = 0?
% 2) Adjust the leakage somehow.
normE = E ./ (exE + 1e-10);

% calculate the reconstructed stuff.
R = zeros(size(B));
for k = 1:24
  R(:, k) = filter(b(k, :), a(k, :), excitation) .* normE(:, k);
end

% normalize to max amplitude = 1 (should be done differently...)
reconstructed = reconstructed(imarkers(1):end);
reconstructed = reconstructed ./ max(abs(reconstructed));