function R = soundGenerator(baseNote, numNotes)
% R = soundGenerator(baseNote, numNotes)
%
% Generates the sound parameters from scratch.
% I will start small (one line, ...)

if (nargin < 1) baseNote = 800; end
if (nargin < 2) numNotes = 10; end

minNote = 100;
maxNote = 6000;
notes = zeros(numNotes, 1);
notes(1) = baseNote;
toneDeviators = [1/4, 1/3, 1/2, 2/5, 2/3, 3/5, 3/4, 4/3, 5/3, 3/2, 5/2, 2, 3, 4];

for newNoteID = 2:numNotes
	bestNote = 0;
	bestNoteError = 1e10;
	oldNotes = notes(1:newNoteID-1);
	newNotes = oldNotes * toneDeviators;
	for newNote = newNotes(:)'
		if (newNote > minNote && newNote < maxNote &&
		    length(find(abs(newNote-oldNotes) < 1e-5)) == 0)
			curError = 0;
			for k = 1:length(oldNotes)
				curError = curError + ...
					errormetric(newNote, oldNotes(k));
			end
			if (curError < bestNoteError)
				bestNote = newNote;
				bestNoteError = curError;
			end
		end
	end
	notes(newNoteID) = bestNote;
end

R = notes;