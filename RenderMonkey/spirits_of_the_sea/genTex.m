function R = genTex(width, height, powersize, numMountains, xmin, xmax, alpha)
% R = genTex(width, height, powersize, numMountains, xmin, xmax, alpha)

R = zeros(width, height);

% variables for pareto size distribution
if (nargin < 7) alpha = 2; end
if (nargin < 5) xmin = 4; end
if (nargin < 6) xmax = 80; end
xmax = min(xmax, min(width, height)./2);

% variables for size-power (positive = low-pass, negative = high-pass)
if (nargin < 3) powersize = 0.3; end

if (nargin < 4) numMountains = 40000; end
mountainPos = rand(2, numMountains) .* repmat([width; height], 1, numMountains);
mountainDir = rand(2, numMountains) - 0.5;
mountainDir = mountainDir ./ repmat(sqrt(sum(mountainDir.*mountainDir)), 2, 1);
%mountainSize = rand(numMountains) .* min(width, height) ./ 2.5;
mountainSize = xmin ./ ((rand(numMountains, 1)+1.0e-10).^(1./alpha));
mountainSize = min(mountainSize, xmax);
%mountainSize = (rand(1, numMountains).^4) .* (xmax-xmin) + xmin;
[u, v] = hist(mountainSize, [2, 4, 8, 16, 32, 64, 128])

for mountain = 1:numMountains
    mPos = mountainPos(:, mountain);
	mDir = mountainDir(:, mountain);
	mSize = mountainSize(mountain);
	mPower = mSize .^ powersize;

	for x = floor(mPos(1)-mSize):ceil(mPos(1)+mSize)
		for y = floor(mPos(2)-mSize):ceil(mPos(2)+mSize)
			
			relPos = [x-mPos(1); y-mPos(2)];
			relDist = sqrt(sum(relPos.*relPos)) ./ mSize;
			relAlpha = sum(relPos.*mDir) * mPower;
			if (relDist <= 1.0)
				%xp = mod(x + width - 1, width) + 1;
				%yp = mod(y + height - 1, height) + 1;
				xp = x;
				yp = y;
				if (xp < 1) xp += width; end
				if (xp > width) xp -= width; end
				if (yp < 1) yp += height; end
				if (yp > height) yp -= height; end
				R(xp, yp) += relAlpha * (relDist*relDist*relDist*relDist - 2*relDist*relDist + 1);
			end
		end
	end
end

