function R = loadPointProcess(filename)
## R = loadPointProcess(filename)
##
## Loads a _short_ PointProcess file
## Converts the markers to 44100 Hz samples.

R = textread(filename, '%f', 'headerlines', 6);
R = R .* 44100;

  