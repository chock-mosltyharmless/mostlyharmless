function tgaWrite(filename, img)
## tgaWrite(filename, img)

fid = fopen(filename, 'wb');

% write header
fwrite(fid, int8(0), 'int8');
fwrite(fid, int8(0), 'int8');
fwrite(fid, int8(2), 'int8');

fwrite(fid, int16(0), 'int16');
fwrite(fid, int16(0), 'int16');
fwrite(fid, int8(0), 'int8');

fwrite(fid, int16(0), 'int16');
fwrite(fid, int16(0), 'int16');
fwrite(fid, int16(size(img, 2)), 'int16');
fwrite(fid, int16(size(img, 1)), 'int16');
fwrite(fid, int8(24), 'int8');
fwrite(fid, int8(0), 'int8');

%imsc(img(:, :, 1));

% write data
%fwrite(fid, int8(img(:)*255), 'int8');
for x = 1:size(img, 1)
  for y = 1:size(img, 2)
    fwrite(fid, uint8(img(x, y, :)), 'uint8');
  end 
end 

fclose(fid);