%filename = 'HUMAN_ACAPELLA_SHO_DO_WOP_DO_AHH_01';
%filename = 'HUMAN_ELEMENT_MALE_HALLEUJAH_LONG_01';
filename = 'HUMAN_VOICE_SING_FIESTA_01';
%filename = 'chok1';
%voiced = true; % extract only voiced component.

%OOOOHHH!!! I need to check the values of voiced and unvoiced shit...

% Constants
FFTLength = 1024; % The length of the main FFT thingie.
intfactor = 1.0; % The larger, the less quantisation error

%barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
%               1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
%               4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
%barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
%               1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
%               4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
barkCenters = [10, 105, 250, 400, 570, 780, 1000, ...
               1275, 1600, 2110, 2510, 3175, ...
               4050, 5350, 7050, 9675, 13750];
%bark = (1.45 .^ (0:15)) * 70;
%barkCenters = (1.375 .^ (0:17)) * 70;
%barkCenters(1) = 0;

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

% Write the image for voicedness.
writeFFTImage(A, FFTLength);

% Calculate the bark energy contours.
[voicEnergies, unvoicEnergies] = fftBank(A, FFTLength, filterbank, [filename, '.tga']);

% filter voicedness over time?
%voicedness = filter(hamming(5)./sum(hamming(5)), 1, voicedness);

% calculate voiced and unvoiced energies from energies and voicedness.
%voicEnergies = energies .* exp(voicedFactor*voicedness);
%unvoicEnergies = energies .* (1-exp(unvoicedFactor*voicedness));

% VOICED:
% Adjust the markers so that I can correctly store them.
avgMarkers = averageMarkers(markers, FFTLength/2, size(voicEnergies, 1));

% Calculate the new averaged marker positions
markers = fromAvgMarkers(avgMarkers, FFTLength/2);

% Create the excitation (voiced)
exec = pulsetrain(markers, size(A)); % energy is around 30.
%exec = klatttrain(markers, size(A), 0.5) ./ 10; % energy is around 30.

% calculate the reference energies:
[refEnergies, tmp] = fftBank(exec, FFTLength, filterbank);

% Calculate the normalized energies. Here, I indeed, I have to work...
voicNormEnergies = voicEnergies ./ (refEnergies + 0.00001);
%voicNormEnergies = voicEnergies;

% Go to int and back
%voicIntEnergies = floor(intfactor*log2(voicNormEnergies));
%voicNormEnergies = 2.^(voicIntEnergies./intfactor);

% apply filterbank to voiced excitation
%voicReconstructed = applyBank(exec, voicNormEnergies, FFTLength, filterbank);
voicReconstructed = applyIIRBank(exec, voicNormEnergies, FFTLength/4, barkCenters);

%UNVOICED:
% excitation source.
exec = (rand(size(A))-0.5)./8; %average energy per bank is 8 -> *4 => 30

% Well... I do the old crappy way of estimating the normEnergies
%refEnergies = fftBank(exec, FFTLength, filterbank);
%unvoicNormEnergies = unvoicEnergies ./ (refEnergies + 0.00001);
unvoicNormEnergies = unvoicEnergies;

%unvoicReconstructed = applyBank(exec, unvoicNormEnergies, FFTLength, filterbank);
unvoicReconstructed = applyIIRBank(exec, unvoicNormEnergies, FFTLength/4, barkCenters);

% COMBINE:
reconstructed = unvoicReconstructed + voicReconstructed;