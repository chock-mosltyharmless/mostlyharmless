function img = tgaRead(filename)
## img = tgaRead(filename)

filename
fid = fopen(filename, 'rb');

% read header
identsize = fread(fid, 1, 'int8') %identsize
fread(fid, 1, 'int8') %colourmaptype
fread(fid, 1, 'int8') %imagetype

fread(fid, 1, 'int16') %colourmapstart
fread(fid, 1, 'int16') %colourmaplength
fread(fid, 1, 'int8') %colourmapbits

fread(fid, 1, 'int16') %xstart
fread(fid, 1, 'int16') %ystart
width = fread(fid, 1, 'int16')
height = fread(fid, 1, 'int16')
fread(fid, 1, 'int8') %bits
desc = fread(fid, 1, 'int8') %descriptor

% add header
fread(fid, identsize, 'int8');

img = zeros(height, width, 3);

% read data
if (desc~=0)
  for x = size(img, 1):-1:1
    for y = 1:size(img, 2)    
      img(x, y, :) = double(fread(fid, 3, 'uint8')) ./ 255;
    end
  end
else
  for x = 1:size(img, 1)
    for y = 1:size(img, 2)
      img(x, y, :) = double(fread(fid, 3, 'uint8')) ./ 255;
    end
  end 
end 
%img(:) = double(fread(fid, width*height*3, 'uint8'));

fclose(fid);