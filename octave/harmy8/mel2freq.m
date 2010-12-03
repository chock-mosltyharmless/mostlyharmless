function f = mel2freq(m)
## f = mel2freq(m)
## 
## Converts from (my modified) mel to frequency in Hz.

f = 1024*(2.^m - 1);
