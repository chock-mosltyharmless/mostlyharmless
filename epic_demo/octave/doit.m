% recode a .tga as a header file that can be included.

w = 960
h = 144

img = tgaRead('../epicfont48.tga');

fid = fopen('font.h', 'w');

% runlength compression of the whole thing.
coreData = reshape(img, 4*h*w, 1);

% The length values
fprintf(fid, 'unsigned short fontLength[48227] = {\n');
oldValue = coreData(1);
position = 2;
length = 1;
numValues = 1;
while (position < w*h*4)
	if (coreData(position) == oldValue)
		length++;
	else
		fprintf(fid, '  %d,\n', length);
		length = 1;
		oldValue = coreData(position);
		numValues = numValues + 1;
	end
	position = position+1;
end
fprintf(fid, '  %d};\n\n', length);
% The value values
fprintf(fid, 'unsigned char fontValues[48227] = {\n');
oldValue = coreData(1);
position = 2;
while (position < w*h*4)
	if (coreData(position) ~= oldValue)
		fprintf(fid, '  %d,\n', oldValue);
		oldValue = coreData(position);
	end
	position = position+1;
end
fprintf(fid, '  %d};\n\n', oldValue);

if (false)
	fprintf(fid, 'unsigned char fontCompressed[4][144][960] = {\n');
	for color = 1:4
		fprintf(fid, '  {\n');
		for y = 1:w
			fprintf(fid, '    {');
			for x = 1:h
				fprintf(fid, '%d,', img(y,x,color));
			end
			fprintf(fid, '},\n');
		end
		fprintf(fid, '  },\n');
	end
	fprintf(fid, '};\n');
end
	
fclose(fid);