function R = writeEnergiesImage(voicIntEnergies, unvoicIntEnergies, avgMarkers, filename);
  
energies = [voicIntEnergies'*6+120; unvoicIntEnergies'*6+120; avgMarkers'];

img = zeros(size(energies, 1), size(energies, 2), 3);
img(:, :, 1) = energies(:, :);
img(:, :, 2) = energies(:, :);
img(:, :, 3) = energies(:, :);

tgaWrite(filename, double(img)/255);