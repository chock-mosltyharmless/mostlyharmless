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
fwrite(fid, int16(size(img, 1)), 'int16');
fwrite(fid, int16(size(img, 2)), 'int16');
fwrite(fid, int8(24), 'int8');
fwrite(fid, int8(0), 'int8');

% write data
fwrite(fid, uint8(img(:)*255), 'uint8');

fclose(fid);