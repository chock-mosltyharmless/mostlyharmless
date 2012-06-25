function R = rotate(A, rotAngle)
% R = rotate(A, rotAngle)
%
% Rotates a 2D signal with a rotation around
% (0,0) specified by the angle.

rotMatrix = [cos(rotAngle), -sin(rotAngle);...
			 sin(rotAngle), cos(rotAngle)];
			 
R = A * rotMatrix;