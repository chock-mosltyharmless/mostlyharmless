function createCHeader(filename, avgMarkers, voicEnergies, unvoicEnergies)
  
fid = fopen(filename, 'w');

% diffMarkers
% First 128 is removed, then diff is marked (max. 10 octaves jump? oh, yeah).
% To reconstruct, add 128 to the first value, then continue as normal.
values = [avgMarkers(1)-128; diff(avgMarkers)];
takeover = 0;
fprintf(fid, 'signed char diffMarkers[%d] = {\n', length(values));

for k = 1:length(values);
  curVal = takeover + values(k);
  takeover = 0;
  if (curVal < -128)
    disp('diffMarkers overflow!')
    takeover = curVal + 128;
    curVal = -128;
  end   
  if (curVal > 127)
    disp('diffMarkers overflow!');
    takeover = curVal - 127;
    curVal = 127;
  end
  fprintf(fid, '    %d,\n', curVal);
end
fprintf(fid, '};\n\n');

% diffVoicIntEnergies:
values = [voicEnergies(1, :); diff(voicEnergies)];
values = [values(:, end), diff(fliplr(values)')'];
fprintf(fid, 'signed char diffVoicEnergies[%d][%d] = {\n', ...
        size(values, 1), size(values, 2));
for k = 1:size(values, 1)
  fprintf(fid, '    {');
  for l = 1:size(values, 2)
    fprintf(fid, '%+d, ', values(k, l));
  end
  fprintf(fid, '},\n');
end
fprintf(fid, '};\n\n');

% diffUnvoicIntEnergies:
values = [unvoicEnergies(1, :); diff(unvoicEnergies)];
values = [values(:, end), diff(fliplr(values)')'];
fprintf(fid, 'signed char diffUnvoicEnergies[%d][%d] = {\n', ...
        size(values, 1), size(values, 2));
for k = 1:size(values, 1)
  fprintf(fid, '    {');
  for l = 1:size(values, 2)
    fprintf(fid, '%+d, ', values(k, l));
  end
  fprintf(fid, '},\n');
end
fprintf(fid, '};\n\n');

fclose(fid);