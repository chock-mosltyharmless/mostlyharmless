numSolid = 1
numCopy = 1
picSize = 60

sizeSolid =...
	[0.2]
posSolid = ...
	[-0.4; -0.4; 1.0]
rotCopy = ...
	[1.7]
sizeCopy =...
	[0.4]
posCopy = ...
	[0.2; 0.2; 1.0]
	

picture = zeros(picSize, picSize);
for x = 1:picSize
	for y = 1:picSize
		curPos = ...
			[(2.0 * x - picSize) / picSize; ...
			 (2.0 * y - picSize) / picSize; ...
			 1.0];
		
		transMatrix = eye(3);
		oldDistSolid = 10.0;
		
		for iteration = 1:4
			% Calculate the size reduction factor
			sizeModifier = sqrt(sum(transMatrix(1:2).*transMatrix(1:2)));
						
			relSolid = transMatrix*posSolid - curPos;
			distSolid = sqrt(sum(relSolid(1:2).*relSolid(1:2)));
			distSolid -= sizeSolid * sizeModifier;
		
			relCopy = transMatrix*posCopy - curPos;
			distCopy = sqrt(sum(relCopy(1:2).*relCopy(1:2)));
			distCopy -= sizeCopy * sizeModifier;
		
			picture(y, x) = min([oldDistSolid, distSolid, distCopy]);
			oldDistSolid = min(oldDistSolid, distSolid);
			
			newTransform = ...
				[cos(rotCopy)*sizeCopy, -sin(rotCopy)*sizeCopy, posCopy(1);...
				 sin(rotCopy)*sizeCopy, cos(rotCopy)*sizeCopy, posCopy(2);...
				 0.0, 0.0, 1.0];
			transMatrix = transMatrix * newTransform;
			%newMove = transMatrix * posCopy;
			%transMatrix(1:3, 3) = newMove;
		end
	end
end