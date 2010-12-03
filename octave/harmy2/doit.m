%filename = 'HUMAN_ACAPELLA_SHO_DO_WOP_DO_AHH_01';
%filename = 'HUMAN_ELEMENT_MALE_HALLEUJAH_LONG_01';
filename = 'chok1';
voiced = true; % extract only voiced component.

% Constants
FFTLength = 2048; % The length of the main FFT thingie.
voicedFactor = 0.0;
unvoicedFactor = 0.0;
intfactor = 1.0; % The larger, the less quantisation error

barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
               1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
               4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
%bark = (1.45 .^ (0:15)) * 70;
barkCenters = (1.375 .^ (0:17)) * 70;
barkCenters(1) = 0;

% Load the sound and point process.
orig = wavread([filename, '.wav']);
orig = orig(1:end-30, 1);
%orig = sin((1:70658)/44100*410*2*pi)';
markers = loadPointProcess([filename, '.PointProcess']);

% Now I should filter everything non-harmonic away?
%A = voicedCorr(orig, 5, markers, FFTLength);
A = orig;

% calculate the FFT filterbank coefficients
filterbank = barkFilterCoefs(FFTLength, barkCenters);

% Calculate the bark energy contours.
[energies, voicedness] = fftBank(A, FFTLength, filterbank);

% filter voicedness over time?
voicedness = filter(hamming(5)./sum(hamming(5)), 1, voicedness);

% voicedness is 1 below and 0 above some freq.
%voicedness = [zeros(size(voicedness, 1), 12), -10 * ones(size(voicedness, 1), size(voicedness, 2) - 12)];

% calculate voiced and unvoiced energies from energies and voicedness.
voicEnergies = energies .* exp(voicedFactor*voicedness);
unvoicEnergies = energies .* (1-exp(unvoicedFactor*voicedness));

% VOICED:
% Adjust the markers so that I can correctly store them.
avgMarkers = averageMarkers(markers, FFTLength/2, size(energies, 1));

% Calculate the new averaged marker positions
markers = fromAvgMarkers(avgMarkers, FFTLength/2);

% Create the excitation (voiced)
exec = pulsetrain(markers, size(A));
%exec = rand(size(A), 1);

% calculate the reference energies:
refEnergies = fftBank(exec, FFTLength, filterbank);

% Calculate the normalized energies. Here, I indeed, I have to work...
%voicNormEnergies = voicEnergies ./ (refEnergies + 0.00001);
voicNormEnergies = voicEnergies;

% Go to int and back
%voicIntEnergies = floor(intfactor*log2(voicNormEnergies));
%voicNormEnergies = 2.^(voicIntEnergies./intfactor);

% apply filterbank to voiced excitation
%voicReconstructed = applyBank(exec, voicNormEnergies, FFTLength, filterbank);
voicReconstructed = applyIIRBank(exec, voicNormEnergies, FFTLength/2, barkCenters);

%UNVOICED:
% excitation source.
exec = rand(size(A))./10;

% Well... I do the old crappy way of estimating the normEnergies
refEnergies = fftBank(exec, FFTLength, filterbank);
%unvoicNormEnergies = unvoicEnergies ./ (refEnergies + 0.00001);
unvoicNormEnergies = unvoicEnergies;

%unvoicReconstructed = applyBank(exec, unvoicNormEnergies, FFTLength, filterbank);
unvoicReconstructed = applyIIRBank(exec, unvoicNormEnergies, FFTLength/2, barkCenters);

% COMBINE:
reconstructed = unvoicReconstructed + voicReconstructed;