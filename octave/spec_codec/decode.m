% Resynthesize original signal from compressed one.
% This Makro assumes that all the compressed data is lying around somewhere

% I am now assuming that I have the diffPhaseSpec and dbSpec
% in unquantized form.
numFrames = size(dbSpec, 2);
FFTSize = size(dbSpec, 1) * 2;
jumpSize = FFTSize / 2;

% reconstruct the normal phase
recPhaseSpec = diffPhaseSpec;
for k = 1:(numFrames-1)
	recPhaseSpec(:,k+1) = recPhaseSpec(:,k) + diffPhaseSpec(:,k+1);
end
% get to -pi:pi
recPhaseSpec = mod(recPhaseSpec, 2 * pi);
recPhaseSpec(find(recPhaseSpec > pi)) = recPhaseSpec(find(recPhaseSpec > pi)) - 2*pi;

% use random phases instead. This works, but I need a general approach to detect noise...
%recPhaseSpec(101:end,:) = rand(412, numFrames) * 2 * pi - pi;

% reconstruct magnitude spectrum from log Spectrum
recMagSpec = 10 .^ (dbSpec ./ 20);

% reconstruct complex spectrum
recHalfSpec = recMagSpec .* exp(i * recPhaseSpec);
recSpec = [recHalfSpec; zeros(1, numFrames); conj(flipud(recHalfSpec(2:end,:)))];

% IFFT to get the individual frames
recFrames = real(ifft(recSpec));

% apply window a second time
win = hann(FFTLength);
win(FFTLength/2+1:end) = 1 - win(1:FFTLength/2);
win = sqrt(win);
recFrames = recFrames .* (repmat(win, 1, numFrames));

% Add up the frames
reconstructed = zeros(jumpSize*numFrames + FFTSize/2, 1);
for frame = 0:numFrames-1
	reconstructed((frame*jumpSize+1):(frame*jumpSize+FFTSize)) = ...
		reconstructed((frame*jumpSize+1):(frame*jumpSize+FFTSize)) + ...
		recFrames(:, frame+1);
end
