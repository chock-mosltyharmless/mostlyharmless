function [voicIntEnergies, unvoicIntEnergies, avgMarkers] = loadEnergiesImage(filename);

energies = tgaRead(filename);
energ = energies(1:end-1, :, 1) * 255 / 6 - 20;
energ = reshape(energ, size(energ, 1), size(energ, 2));

voicIntEnergies = floor(energ(1:size(energ)/2, :)'+0.5);
unvoicIntEnergies = floor(energ(size(energ, 1)/2 + 1:end, :)'+0.5);

avgMarkers = floor(energies(end, :, 1)' * 255 + 0.5);