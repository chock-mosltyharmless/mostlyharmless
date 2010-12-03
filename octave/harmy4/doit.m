function reconstructed = doit(filename, loadEnergies, modus, fixedAvgMarkers)
if (nargin < 1)
  %filename = 'voice/HUMAN_ACAPELLA_SHO_DO_WOP_DO_AHH_01';
  %filename = 'test';
  %filename = 'voice/HUMAN_VOICE_SING_FIESTA_01';
  filename = 'voice/atomicplayboy2';
  %filename = 'stringInstruments/bass2';
  %filename = 'drums/drum1';
  %filename = 'tmp-c';
end   

%OOOOHHH!!! I need to check the values of voiced and unvoiced shit...

% Constants
FFTLength = 2048; % The length of the main FFT thingie.
stepSize = FFTLength/4;
%intfactor = 0.5; % The larger, the less quantisation error (.5 being the minimum)
intfactor = 2.0; % The larger, the less quantisation error (.5 being the minimum)
%markerIntFactor = 24; % The larger, the less quantisation error. (12 being minimum)
markerIntFactor = 24; % The larger, the less quantisation error. (12 being minimum)
if (nargin < 3)
  modus = 3; % 1: unvoiced, 2: voiced, 3: both
end
if (nargin < 2)
  loadEnergies = true;
end
%fixedAvgMarkers = 0/44100*FFTLength; % A value larger 0 means the position in the spectrogram (from 0)
if (nargin < 4)
  fixedAvgMarkers = 0/2048*FFTLength; % A value larger 0 means the position in the spectrogram (from 0)
end
pitchShift = 0;

%barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
%               1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
%               4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
%barkCenters = [0, 60, 150, 250, 350, 455, 570, 700, 845, 1000, ...
%               1175, 1375, 1600, 1860, 2160, 2510, 2925, 3425, ...
%               4050, 4850, 5850, 7050, 8600, 10750, 13750, 19000];
%barkCenters = [10, 105, 250, 400, 570, 780, 1000, ...
%               1275, 1600, 2110, 2510, 3175, ...
%               4050, 5350, 7050, 9675, 13750];
%barkCenters = [10, 80, 225, 380, 545, 740, 965, 1230, 1550, 1900, ...
%               2430, 3050, 3880, 5100, 7200, 11100, 16000];
%barkBorders = [10, 150, 300, 460, 630, 850, 1080, 1380, 1720, ...
%               2160, 2700, 3380, 4400, 5750, 8600, 13750];
barkCenters = [5, 55, 150, 260, 390, 545, 740, 965, 1200, 1550, ...
               1940, 2430, 3040, 3890, 5075, 7175, 11175, 16000];
barkBorders = [10, 100, 200, 320, 460, 630, 850, 1080, 1380, 1720, ...
               2160, 2700, 3380, 4400, 5750, 8600, 13750];
%bark = (1.45 .^ (0:15)) * 70;
%barkCenters = (1.375 .^ (0:17)) * 70;
%barkCenters(1) = 0;

% Load the sound and point process.
orig = wavread([filename, '.wav']);
%orig = orig(:, 1);
orig = orig(1:end-30, 1);
A = orig;

if (~loadEnergies)  
  % Now I should filter everything non-harmonic away?
  %A = voicedCorr(orig, 5, markers, FFTLength);

  % calculate the FFT filterbank coefficients
  %filterbank = barkFilterCoefs(FFTLength, barkCenters);
  filterbank = barkFilterCoefs(FFTLength, barkBorders);

  if (modus == 3)
    % Write the image for voicedness.
    writeFFTImage(A, FFTLength);

    % Calculate the bark energy contours.
    [voicEnergies, unvoicEnergies] = fftBank(A, FFTLength, filterbank, [filename, '.tga']);
  else
    if (modus == 1)
      [unvoicEnergies, voicEnergies] = fftBank(A, FFTLength, filterbank);
    else
      [voicEnergies, unvoicEnergies] = fftBank(A, FFTLength, filterbank);
    end
  end

  % VOICED:
  % Adjust the markers so that I can correctly store them.
  if (fixedAvgMarkers == 0)
    markers = loadPointProcess([filename, '.PointProcess']);
    avgMarkers = averageMarkers(markers, stepSize, size(voicEnergies, 1));
  else
    avgMarkers = repmat(FFTLength / fixedAvgMarkers, size(voicEnergies, 1), 1);
  end
  
  % go int and back.
  intAvgMarkers = floor(log2(avgMarkers) * markerIntFactor);

  % Calculate the reference energies from markers directly...
  %markers = fromAvgMarkers(...
  %  ...%[avgMarkers(1); avgMarkers(1); avgMarkers; avgMarkers(end); avgMarkers(end)], ...
  %  ...%[avgMarkers(2:end); repmat(avgMarkers(end), 5, 1)], ...
  %  avgMarkers,
  %  stepSize);
  %exec = pulsetrain(markers, size(A)) ./ 30; % energy is around 30.

  % calculate the reference energies:
  %[refEnergies, tmp] = fftBank(exec, FFTLength, filterbank);
  % Calculate the normalized energies. Here, I indeed, I have to work...
  %voicNormEnergies = voicEnergies ./ (refEnergies + 1e-4);
  voicNormEnergies = voicEnergies;
  % Go to int
  voicIntEnergies = floor(intfactor*log2(voicNormEnergies+1e-3));
  
  % Well... I do the old crappy way of estimating the normEnergies
  %refEnergies = fftBank(exec, FFTLength, filterbank);
  %unvoicNormEnergies = unvoicEnergies ./ (refEnergies + 0.00001);
  unvoicNormEnergies = unvoicEnergies;
  % Go to int
  unvoicIntEnergies = floor(intfactor*log2(unvoicNormEnergies+1e-3));
  % write the energies to tga
  writeEnergiesImage(voicIntEnergies, unvoicIntEnergies, intAvgMarkers, ...
                    [filename, '-energies.tga']);
else
  [voicIntEnergies, unvoicIntEnergies, intAvgMarkers] = ...
    loadEnergiesImage([filename, '-energies.tga']);
end

% Calculate the new averaged marker positions
avgMarkers = 2.^((intAvgMarkers+pitchShift)./markerIntFactor);
markers = fromAvgMarkers(avgMarkers(1:end), stepSize);

%markers = loadPointProcess([filename, '.PointProcess']);

% Filter energies
unvoicIntEnergies = temporalFilter(unvoicIntEnergies);
voicIntEnergies = temporalFilter(voicIntEnergies);
unvoicIntEnergies = temporalFilter(unvoicIntEnergies')';
voicIntEnergies = temporalFilter(voicIntEnergies')';

unvoicNormEnergies = 2.^(unvoicIntEnergies./intfactor);
voicNormEnergies = 2.^(voicIntEnergies./intfactor);

% Apply 4 silence blocks first, so that the filters can adopt.
%unvoicNormEnergies = [zeros(4, size(unvoicNormEnergies, 2)); unvoicNormEnergies];
%voicNormEnergies = [zeros(4, size(voicNormEnergies, 2)); voicNormEnergies];

% Create the excitation (voiced)
exec = pulsetrain(markers, size(A)) ./ 30; % energy is around 30.
%exec = pulsetrain(markers + 4*stepSize, size(A) + [4*stepSize, 0]) ./ 30; % energy is around 30.
%exec = klatttrain(markers, size(A), 0.5) ./ 10; % energy is around 30.

% apply filterbank to voiced excitation
%voicReconstructed = applyBank(exec, voicNormEnergies, FFTLength, filterbank);
voicReconstructed = applyIIRBank(exec, voicNormEnergies, stepSize, barkBorders);

%UNVOICED:
% excitation source.
exec = (rand(size(A))-0.5)./3; %average energy per bank is 8 -> *4 => 30
%exec = (rand(size(A) + [4*stepSize, 0])-0.5)./4; %average energy per bank is 8 -> *4 => 30

%unvoicReconstructed = applyBank(exec, unvoicNormEnergies, FFTLength, filterbank);
unvoicReconstructed = applyIIRBank(exec, unvoicNormEnergies, stepSize, barkBorders);

% COMBINE:
reconstructed = (unvoicReconstructed + voicReconstructed) ./ 2;
%reconstructed = (voicReconstructed) ./ 2;

% Create C header data:
createCHeader([filename, '.h'], intAvgMarkers, ...
              voicIntEnergies, unvoicIntEnergies);
%              
% save
%beginStrip = min(find(abs(reconstructed > 0.01)));
%endStrip = max(find(abs(reconstructed > 0.01)));
%stripped = reconstructed(beginStrip:endStrip);
stripped = reconstructed;
wavwrite(stripped, 44100, 16, [filename, '.joh.wav']);