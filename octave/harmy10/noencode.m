% well... we start with pitch
%doPitch
% Now I've got pitch...
chosenPitch = 1; %Choose the one you see fit. Often 1!
voicednessDrop = 0.01;
%voicednessDrop = 0.0;
numOvertones = 128;
%numBands = 20;
%bandScale = 4;
numBands = 12;
bandScale = 2;

% Floor the pitch to 43 Hz.
pitch = min(pitch, FFTLength/2);

% Add entries so that it adds up to 1...
spec = spgram(orig, FFTLength, frameStep);
aspec = abs(spec);
numFrames = size(spec, 2);

% Get the FFT locations for the pitches
pitchVal = zeros(numOvertones, numFrames); % up to 64 overtones
for frame = 1:numFrames
	for overtone = 1:numOvertones
		pitchVal(overtone, frame) = ...
			pitch(frame, chosenPitch)./overtone;
	end
end
FFTCenters = pitch2FFTIndex(pitchVal, FFTLength);

% The arrays go to the very ends of the spectrum.
% It might actually be better not to go that far.
% Dunno.
FFTStarts = FFTCenters;
FFTEnds = FFTCenters;
for frame = 1:numFrames
	FFTCF = FFTCenters(:,frame);
	FFTStarts(:,frame) = ...
		[1; floor((FFTCF(1:end-1)+FFTCF(2:end))./2)+1];
	FFTEnds(:,frame) = ...
		[floor((FFTCF(1:end-1)+FFTCF(2:end))./2); FFTLength/2];
end
ebig = find(FFTEnds > FFTLength / 2);
FFTEnds(ebig) = FFTLength / 2;

% Calculate the absolute overtone spectrum
% TODO: This does _not_ work for extremely small pitches!
aospec = zeros(numOvertones, numFrames);
for frame = 1:numFrames
	for overtone = 1:numOvertones
		bins = FFTStarts(overtone,frame):FFTEnds(overtone,frame);
		aospec(overtone, frame) = ...
			sum(aspec(bins, frame));
	end
end

% Combine the overtone spectrum into bands.
curBin = 1;
aoBands = zeros(numBands, numFrames);
for band = 1:numBands
	numBins = 2.^floor((band-1)./bandScale);
	if (numBins > 1)
		aoBands(band, :) = ...
			mean(aospec(curBin:curBin+numBins-1, :));
	else
		aoBands(band, :) = aospec(curBin, :);
	end
	curBin = curBin + numBins;
end

% discretize
laoBands = floor(log2(aoBands));
aoBands = 2.^laoBands;

% change the spec so that it is constant for the bands
aospecOrig = aospec;
aospec = zeros(size(aospec));
curBin = 1;
for band = 1:numBands
	numBins = 2.^floor((band-1)./bandScale);
	aospec(curBin:curBin+numBins-1, :) = ...
		repmat(aoBands(band, :), numBins, 1);
	curBin = curBin + numBins;
end
	
% Reconstruct assuming that I am 100% voiced
signalLength = (numFrames-1) * frameStep;
signal = zeros(signalLength, 1);
for overtone = 2:numOvertones-1
	oAmpl = zeros(signalLength, 1);
	oPitch = zeros(signalLength, 1);
	for frame = 1:numFrames-1
		startAmpl = aospec(overtone, frame);
		endAmpl = aospec(overtone, frame+1);
		startPitch = pitch(frame) ./ overtone;
		endPitch = pitch(frame+1) ./ overtone;
		oAmpl((frame-1)*frameStep+1:frame*frameStep) = ...
			startAmpl + ...
			(1:frameStep)./frameStep * (endAmpl - startAmpl);
		oPitch((frame-1)*frameStep+1:frame*frameStep) = ...
			startPitch + ...
			(1:frameStep)./frameStep * (endPitch - startPitch);
	end
	
	% underestimation of high frequencies
	%oAmpl = oAmpl ./ sqrt(overtone);

	% Calculate some low-freq noise
	noise = rand(signalLength, 1)-0.5;
	noise = filter(1, [1,-1 + 1./128], noise);
	noise = filter(1, [1,-1 + 1./128], noise);
	%noise = filter(1, [1,-1 + 1./128], noise);
	noise = noise ./ 32 ./ 16;

	% Set voicedness to 0 at 16th overtone
	voicedness = 1 - overtone ./ 16;
	voicedness = max(0, voicedness);
	modulation = voicedness + (1.0 - voicedness) * noise;
	
	%synth
	phase = (rand(1) + cumsum(1./oPitch)).*2.*pi;
	signal = signal + sin(phase).*oAmpl.*modulation;
end

%normalize?
signal = signal ./ max(abs(signal));

wavwrite(signal, 44100, 16, [filename,'.joh.wav']);