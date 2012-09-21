filenames = ...
{...
  'samples/andsometimes.wav',...
}

for f = filenames
  filename = f{1}
  noencode
  decode
end