function play(A, Fs)
## play (A, Fs)
## 
## Plays a sound using sndrec32 synchroneously.
## A The column vector that is supposed to be played
## Fs [optional] is the sampling frequency, default 44100
## A wav file tmp.wav is created in the current directory. So what.

if (nargin < 2)
  Fs = 44100;
end   

wavwrite(A, Fs, 16, 'tmp.wav');

% play file through new process
system('sndrec32 /play /close tmp.wav');