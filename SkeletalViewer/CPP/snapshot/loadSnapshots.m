function R = loadSnapshots()
% R = loadSnapshots()

R = {};

for k = 1:10
	fid = fopen(['snapshot', num2str(k),'.sht'], 'r');
	R{k} = fread(fid, [320,240], "uint16=>double");
	fclose(fid);
end