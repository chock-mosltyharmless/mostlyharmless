width = 512
height = 512
alpha = 2.5 % alpha for distribution
xmin = 2
xmax = 64
powersize = 0.1
numMountains = 100000

% parameters for post-processing
filterSize = 32
filterReduction = 0.0

clear R
R(1, :, :) = genTex(width, height, powersize, numMountains, xmin, xmax, alpha);
R(1, :, :) -= 0.8 * max(max(max(R(1, :, :))));
% Highpass
R1 = reshape(R(1,:,:), width, height);
tmp1=filter(hamming(filterSize),1,abs([R1,R1,R1])');
tmp2=filter(hamming(filterSize),1,[tmp1,tmp1,tmp1]');
RL1 = tmp2(width+1+filterSize/2:2*width+filterSize/2, ...
		   height+1+filterSize/2:2*height+filterSize/2);
RF1 = R1./(RL1+filterReduction*mean(mean(RL1)));

%imagesc(RF1)
%return

R(2, :, :) = genTex(width, height, powersize, numMountains, xmin, xmax, alpha);
R(2, :, :) -= 0.8 * max(max(max(R(2, :, :))));
% Highpass
R2 = reshape(R(2,:,:), width, height);
tmp1=filter(hamming(filterSize),1,abs([R2,R2,R2])');
tmp2=filter(hamming(filterSize),1,[tmp1,tmp1,tmp1]');
RL2 = tmp2(width+1+filterSize/2:2*width+filterSize/2, ...
		   height+1+filterSize/2:2*height+filterSize/2);
RF2 = R2./(RL2+filterReduction*mean(mean(RL2)));

R(3, :, :) = genTex(width, height, powersize, numMountains, xmin, xmax, alpha);
R(3, :, :) -= 0.8 * max(max(max(R(3, :, :))));
% Highpass
R3 = reshape(R(3,:,:), width, height);
tmp1=filter(hamming(filterSize),1,abs([R3,R3,R3])');
tmp2=filter(hamming(filterSize),1,[tmp1,tmp1,tmp1]');
RL3 = tmp2(width+1+filterSize/2:2*width+filterSize/2, ...
		   height+1+filterSize/2:2*height+filterSize/2);
RF3 = R3./(RL3+filterReduction*mean(mean(RL3)));

R(4, :, :) = genTex(width, height, powersize, numMountains, xmin, xmax, alpha);
R(4, :, :) -= 0.8 * max(max(max(R(4, :, :))));
% Highpass
R4 = reshape(R(4,:,:), width, height);
tmp1=filter(hamming(filterSize),1,abs([R4,R4,R4])');
tmp2=filter(hamming(filterSize),1,[tmp1,tmp1,tmp1]');
RL4 = tmp2(width+1+filterSize/2:2*width+filterSize/2, ...
		   height+1+filterSize/2:2*height+filterSize/2);
RF4 = R4./(RL4+filterReduction*mean(mean(RL4)));

% copy back
R(1,:,:) = RF1;
R(2,:,:) = RF2;
R(3,:,:) = RF3;
R(4,:,:) = RF4;

R(1, :, :) = R(1, :, :) - min(min(R(1, :, :)));
R(1, :, :) = R(1, :, :) ./ max(max(R(1, :, :)));
R(2, :, :) = R(2, :, :) - min(min(R(2, :, :)));
R(2, :, :) = R(2, :, :) ./ max(max(R(2, :, :)));
R(3, :, :) = R(3, :, :) - min(min(R(3, :, :)));
R(3, :, :) = R(3, :, :) ./ max(max(R(3, :, :)));
R(4, :, :) = R(4, :, :) - min(min(R(4, :, :)));
R(4, :, :) = R(4, :, :) ./ max(max(R(4, :, :)));

%writeDDS('AllOfIt.dds', R);
writeDDSMip('AllOfIt.dds', R);

imagesc([RF1,RF2;RF3,RF4]);