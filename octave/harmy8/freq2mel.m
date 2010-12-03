function m = freq2mel(f)
## m = freq2mel(f)
## 
## Converts from frequency in Hz to (my modified) mel.

m = log2(f./1024 + 1);

