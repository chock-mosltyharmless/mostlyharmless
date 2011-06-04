function writeDDS(filename, img)
## writeDDS(filename, img)
## writes 32 bit...

fid = fopen(filename, 'wb');

% write header
fwrite(fid, int32(542327876), 'int32');     % "DDS "
fwrite(fid, int32(124), 'int32');		    % dwSize
fwrite(fid, int32(4096+7), 'int32');	    % dwFlags
fwrite(fid, int32(size(img,2)), 'int32');   % height
fwrite(fid, int32(size(img,1)), 'int32');   % width
fwrite(fid, int32(size(img,1)*4), 'int32'); % pitch
fwrite(fid, int32(0), 'int32');		        % depth
fwrite(fid, int32(0), 'int32');		        % mipmap
fwrite(fid, int32(1:11), 'int32');	        % reserved[11]
% pixelformat
fwrite(fid, int32(32), 'int32');		    % dwSize
fwrite(fid, int32(65), 'int32');	        % dwFlags
fwrite(fid, int32(0), 'int32');		        % 4cc
fwrite(fid, int32(32), 'int32');	        % bitdepth
fwrite(fid, int32(16711680), 'int32');	    % rmask
fwrite(fid, int32(65280), 'int32');	        % gmask
fwrite(fid, int32(255), 'int32');	        % bmask
fwrite(fid, uint32(4278190080), 'uint32');  % amask
% header continued
fwrite(fid, int32(4096), 'int32');	        % dwCaps
fwrite(fid, int32(0), 'int32');		        % dwCaps2
fwrite(fid, int32(0), 'int32');		        % dwCaps3
fwrite(fid, int32(0), 'int32');		        % dwCaps4
fwrite(fid, int32(0), 'int32');		        % dwReserved2

% write data
fwrite(fid, uint8(img(:)*255), 'uint8');

fclose(fid);

