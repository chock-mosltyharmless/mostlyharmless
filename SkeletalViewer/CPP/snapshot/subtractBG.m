function R = subtractBG(fg, bg)
% function R = subtractBG(fg, bg)

R = fg;
R(find(bg-50*8 < fg)) = 65000;

% remove lr noise fg
for y = 1:240
	lastBG = -1000;
	for x = 1:320
		if (R(x,y) == 65000)
			if (x - lastBG < 5)
				R(lastBG:x, y) = 65000;
			end
			lastBG = x;
		end
	end
end

% remove ud noise fg
for x = 1:320
	lastBG = -1000;
	for y= 1:240
		if (R(x,y) == 65000)
			if (y - lastBG < 5)
				R(x, lastBG:y) = 65000;
			end
			lastBG = y;
		end
	end
end


% remove lr bg...
for y = 1:240
	lastFG = -1000;
	for x = 1:320
		if (R(x,y) < 65000)
			if (x - lastFG == 2)
				R(x-1, y) = mean(R([x-2,x], y));
			end
			if (x - lastFG == 3)
				R(x-2, y) = mean(R([x-3,x-3,x], y));
				R(x-1, y) = mean(R([x-3,x,x], y));
			end
			lastFG = x;
		end
	end
end

% remove ud bg...
for x = 1:320
	lastFG = -1000;
	for y = 1:240
		if (R(x,y) < 65000)
			if (y - lastFG == 2)
				R(x, y-1) = mean(R(x, [y-2,y]));
			end
			if (y - lastFG == 3)
				R(x, y-2) = mean(R(x, [y-3,y-3,y]));
				R(x, y-1) = mean(R(x, [y-3,y,y]));
			end
			lastFG = y;
		end
	end
end