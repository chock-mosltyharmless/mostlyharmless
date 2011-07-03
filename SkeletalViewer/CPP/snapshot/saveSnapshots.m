function saveSnapshots(snapshot)
% saveSnapshots(snapshot)

R = {};

fid = fopen('snapshot.sht', 'w');
fwrite(fid, snapshot, 'uint16');
fclose(fid);
