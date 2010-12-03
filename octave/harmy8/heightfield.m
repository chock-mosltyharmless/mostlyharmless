function r = heightfield(t, f, values, times, mels, timescales, melscales)
## r = heightfield(t, f, values, times, mels, timescales, melscales)
## 
## Calculate the height of a 2D (time-mel) heightfield.
## input:
## t: time (in samples?), may be an array/matrix
## f: frequency (in Hz), may be an array/matrix
## values: the "Height" of the mountains in the heightfield. May be < 0 for valleys
## times: Times of the mountains in samples
## mels: mel frequencies of the mountains
## melscale: The rate of descend for each mountain in mel domain
## timescale: The rate of descend for each mountain in time domain

m = freq2mel(f);

r = zeros(size(t));
for i = 1:length(values)
  timeDist = (t - times(i)) * timescales(i);
  melDist = (m - mels(i)) * melscales(i);
  dist = sqrt(timeDist .* timeDist + melDist .* melDist);
  r(find(dist < 1)) = r(find(dist < 1)) + values(i) * (1-dist(find(dist < 1)));
  %if (dist < 1)
  %  r = r + values(i) * (1-dist);
  %end
end
