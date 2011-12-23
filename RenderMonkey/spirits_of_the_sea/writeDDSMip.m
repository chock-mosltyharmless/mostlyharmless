function writeDDSMip(filename, img)
## writeDDS(filename, img)
## writes 32 bit...

numMipMaps = 1;
minSize = min(size(img, 2), size(img, 3));
while (minSize > 1)
	numMipMaps = numMipMaps + 1;
	minSize = minSize / 2;
end
numMipMaps

fid = fopen(filename, 'wb');

% write header
fwrite(fid, int32(542327876), 'int32');     % "DDS "
fwrite(fid, int32(124), 'int32');		    % dwSize
fwrite(fid, int32(4096+7+131072), 'int32');	    % dwFlags: mipmapping!
fwrite(fid, int32(size(img,3)), 'int32');   % height
fwrite(fid, int32(size(img,2)), 'int32');   % width
fwrite(fid, int32(size(img,2)*4), 'int32'); % pitch
fwrite(fid, int32(0), 'int32');		        % depth
fwrite(fid, int32(numMipMaps), 'int32');		        % mipmap
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
fwrite(fid, int32(4096 + 8 + 4194304), 'int32');	        % dwCaps
fwrite(fid, int32(0), 'int32');		        % dwCaps2
fwrite(fid, int32(0), 'int32');		        % dwCaps3
fwrite(fid, int32(0), 'int32');		        % dwCaps4
fwrite(fid, int32(0), 'int32');		        % dwReserved2

% write no mipmapping data
fwrite(fid, uint8(img(:)*255), 'uint8');

% filter img data
for k = 2:numMipMaps
	width = size(img, 2);
	height = size(img, 3);
	img2 = zeros(4, width/2, height/2);
	for colorComp = 1:4
		for y = 1:2:height
			up = y - 1; if (up < 1) up = height; end
			down = y + 1;
			downdown = y + 2; if (downdown > height) downdown = 1; end
			for x = 1:2:width
				left = x-1; if (left < 1) left = width; end
				right = x+1;
				rightright = x+2; if (rightright > width) rightright = 1; end
				img2(colorComp, (x+1)/2, (y+1)/2) = (...
					img(colorComp, x, y) + ...
					img(colorComp, right, y) + ...
					img(colorComp, x, down) + ...
					img(colorComp, right, down) + ...
					0.5 * img(colorComp, left, y) + ...
					0.5 * img(colorComp, left, down) + ...
					0.5 * img(colorComp, x, downdown) + ...
					0.5 * img(colorComp, right, downdown) + ...
					0.5 * img(colorComp, rightright, down) + ...
					0.5 * img(colorComp, rightright, y) + ...
					0.5 * img(colorComp, right, up) + ...
					0.5 * img(colorComp, x, up) ...
					) ./ 8;
			end
		end
	end
	img = img2;
	
	% write mipmapping data
	fwrite(fid, uint8(img(:)*255), 'uint8');
end

fclose(fid);


