function r = synthesize(numSamples, values, times, mels, timescales, melscales, basefreq, basevoiced)
## r = synthesize(numSamples, values, times, mels, timescales, melscales, basefreq, basevoiced)
## 
## Synthesize a sound based on a heightfield, the frequency and voicedness
## numSamples: Number of samples: 44100 means one second of sound.
## values: the "Height" of the mountains in the heightfield. May be < 0 for valleys
## times: Times of the mountains in samples
## mels: mel frequencies of the mountains
## melscale: The rate of descend for each mountain in mel domain
## timescale: The rate of descend for each mountain in time domain
## basefreq: DC component of frequency in Hz
## basevoiced: DC component of voicedness in [0, 1]
##
## Problems:
## When doing changeable frequencies I need to take care of no jumps...

% Maximum frequency in Hz.
maxFreq = 16000;
sampleFreq = 44100;

noise = rand(numSamples, 32) - 0.5;
for i = 1:3
  noise = filter(0.03125, [1, -0.9875], noise);
end

noise = repmat(noise, 1, 16); % And hope for the best...
r = zeros(numSamples, 1);

for t = 1:numSamples
  % for each sample...
  % calculate the frequencies:
  frequency = basefreq;
  f = (frequency:frequency:maxFreq)';
  
  % get amplitudes from heightfield:  
  A = heightfield(repmat(t, size(f, 1), size(f, 2)), f, values, times, mels, timescales, melscales);
  
  noises = (1.0-basevoiced) * noise(t, 1:length(f)) + basevoiced;
  
  % synthesize (TODO: voicedness, Frequency modulation)
  r(t) = sum((2.^(A-20)) .* sin(t .* f ./ sampleFreq .* 2 .* pi) .* noises');
end
