% Matrices that define the thing
numMatrices = 3
matrix{1} = [1.97, -0.2, -0.85; 0.2, 1.98, -0.75];
matrix{2} = [-1.83, 0.1, 0.65; 0.7, 1.7, -0.65];
matrix{3} = [1.9, -0.1, 0.75; 0.3, 1.76, 0.85];

picture = zeros(400, 400);

for x = 1:400
	for y = 1:400
		startPos = [(x-200) ./ 190; (y-200) ./ 190];
		
		for iter = 1:7
			bestDistance = 1.0e50;
			bestIndex = 0;
			for m = 1:numMatrices
				newPos = matrix{m} * [startPos; 1.0];
				distance = sum(newPos .* newPos);
				if (distance < bestDistance)
					bestDistance = distance;
					bestIndex = m;
				end
			end
			
			startPos = matrix{bestIndex} * [startPos; 1.0];
		end
		
		distance = sum(startPos .* startPos);
		picture(x, y) = sqrt(distance);
	end
end