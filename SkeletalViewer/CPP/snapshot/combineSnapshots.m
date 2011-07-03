function R = combineSnapshots(snapshots)
% R = combineSnapshots

R = zeros(size(snapshots{1}));

for x = 1:320
	for y = 1:240
		distances = [];
		for k = 1:10
			if (snapshots{k}(x, y) > 0)
				distances = [distances; snapshots{k}(x, y)];
			end
		end
		
		if (length(distances) > 0)
			%R(x, y) = median(distances);
			sorted = sort(distances);
			position = floor((length(distances)+1) / 2);
			R(x, y) = sorted(position);
		else
			R(x, y) = 65000;
		end
	end
end