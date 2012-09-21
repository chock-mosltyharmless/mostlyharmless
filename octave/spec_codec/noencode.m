% Parameters
sampleFreq = 44100;
FFTLength = 1024;
frameStep = FFTLength / 2;
minimumLevel = -50;
%frameStep = 512;

% Load file
orig = wavread(filename);
orig = orig(1:end-50, 1);
%orig = randn(22050, 1) ./ 5;

% Use sine function instead, 10th FFT line, 1 second
%frequency = sampleFreq / FFTLength * 10.3;
%t = (1:sampleFreq)' ./ sampleFreq;
%orig = sin(t * frequency * 2 * pi);

% create spectrogram
spec = spgram(orig, FFTLength, frameStep);
numFrames = size(spec, 2);
halfSpec = spec(1:FFTLength/2, :);
magSpec = abs(halfSpec);
% maximum of -50 dB for magnitude spectrum
magSpec = max(magSpec, 10^(minimumLevel/20));
phaseSpec = angle(halfSpec);
dbSpec = 20*log10(magSpec);

% create diff phase spec
%diffPhaseSpec = [phaseSpec(1,:); diff(phaseSpec)];
diffPhaseSpec = [phaseSpec(:, 1), diff(phaseSpec')'];
diffPhaseSpec(find(diffPhaseSpec < 0)) = diffPhaseSpec(find(diffPhaseSpec < 0)) + 2*pi;

% wavwrite(signal, 44100, 16, [filename,'.joh.wav']);