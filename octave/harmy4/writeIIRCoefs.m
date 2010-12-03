function writeIIRCoefs(frequencies)
% writeIIRCoefs(frequencies)
%
% Writes to C header file filter.h

% Butterworth filterband. At first really simple...
% I have to exchange these with the bark filterbanks.
%frequencies = (1.25 .^ (0:24)') * 75;
for k = 1:length(frequencies)-1
  if (k > 1)
    freq1 = frequencies(k) - 0.1 * (frequencies(k+1) - frequencies(k));
  else
    freq1 = frequencies(k);
  end
  freq2 = frequencies(k+1) + 0.1 * (frequencies(k+1) - frequencies(k));
  [b, a] = butter(3, [freq1./22050, freq2./22050]);  
  %[b, a] = butter(3, [frequencies(k)./22050, frequencies(k+1)./22050]);
  % Go to pole/zero
  [z, p, g] = tf2zp(b, a);
  [b1(k, :), a1(k, :)] = zp2tf([-1; 1], p(1:2), g ^ (1/3));
  %[b1(k, :), a1(k, :)] = zp2tf([-1; 1], p(1:2), 1);
  [b2(k, :), a2(k, :)] = zp2tf([-1; 1], p(3:4), g ^ (1/3));
  %[b2(k, :), a2(k, :)] = zp2tf([-1; 1], p(3:4), 1);
  [b3(k, :), a3(k, :)] = zp2tf([-1; 1], p(5:6), g ^ (1/3));
  %[b3(k, :), a3(k, :)] = zp2tf([-1; 1], p(5:6), 1);
end

%b1
%a1
%a2
%b3
%a3

% Write
fid = fopen('filter.h', 'w');
fprintf(fid, 'float barkB[%d][%d] = {\n', length(frequencies)-1, 3);
for k = 1:length(frequencies) - 1
  fprintf(fid, '  {%.20ff, %.20ff, %.20ff}, \n', b1(k, 1), b2(k, 1), b3(k, 1));
end
fprintf(fid, '};\n\n');

fprintf(fid, 'float barkA2[%d][%d] = {\n', length(frequencies)-1, 3);
for k = 1:length(frequencies) - 1
  fprintf(fid, '  {%.20ff, %.20ff, %.20ff}, \n', a1(k, 2), a2(k, 2), a3(k, 2));  
end
fprintf(fid, '};\n');
fprintf(fid, 'float barkA3[%d][%d] = {\n', length(frequencies)-1, 3);
for k = 1:length(frequencies) - 1
  fprintf(fid, '  {%.20ff, %.20ff, %.20ff}, \n', a1(k, 3), a2(k, 3), a3(k, 3));  
end
fprintf(fid, '};\n');

fclose(fid);