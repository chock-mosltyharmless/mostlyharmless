function [matrix, z, p, g] = create()

% $$$   bark = [20, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, ...
% $$$           1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, ...
% $$$           7700, 9500, 12000, 15500];
%$$$  bark = (1.25 .^ (0:24)) * 75;
  bark = (1.45 .^ (0:15)) * 70;
  bark(1) = 20;
% $$$   bark = [20, 107, 192, 290, 400, 510, 630, 770, 920, 1080, 1270, ...
% $$$           1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, ...
% $$$           7700, 9500, 12000, 15500];
% $$$   bark = [20, 200, 400, 800, 1600, 3000, 5000, 8000, 14000]
  
  fid1 = fopen('ba.txt', 'w');
  fid2 = fopen('zpg.txt', 'w');

  z = zeros(length(bark)-1, 2);
  p = zeros(length(bark)-1, 2);
  g = zeros(length(bark)-1, 1);  
  
  for k = 1:length(bark)-1
    freq1 = bark(k) - 0.5 * (bark(k+1) - bark(k));
    freq1 = max(freq1, 5)
    freq2 = bark(k+1) + 0.5 * (bark(k+1) - bark(k));
    %freq1 = bark(k);
    %freq2 = bark(k+1);
    [b,a] = butter(1, [freq1/22050, freq2/22050]);
    %[b,a] = butter(2, [freq1/22050]);
    [z(k,:),p(k,:),g(k)] = tf2zp(b,a);

    h(:, k) = freqz(b, a, 22050);    
    
    fprintf(fid1, '%.10f %0.10f %0.10f; ', b(1), b(2), b(3));
    fprintf(fid1, '%.10f %0.10f %0.10f\n', a(1), a(2), a(3));
    fprintf(fid2, '%.10f %0.10f; %.10f %0.10f; %.10f\n', z, real(p(1)), ...
            imag(p(1)), g);
  end

  disph = max(h, ones(size(h)) * 0.01);
  
  plot(20*log10(abs(disph(1:20000,1:10)).^5));
  %plot(20*log10(sum((abs(disph(22:4000,:)).^5)')'));

  % create the 5-matrix:
  %for k = 1:size(h, 2)
%    for l = 1:size(h, 2)
%      % entry = volumen unter dem bin?
%      matrix(k, l) = mean(abs(h(bark(l):bark(l+1)-1, k).^1));
%    end
%  end

  fclose(fid1);
  fclose(fid2);