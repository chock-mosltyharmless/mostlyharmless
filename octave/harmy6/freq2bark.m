function bark=freq2bark(freq)
%bark=freq2bark(freq)

bark = 13*arctan(0.00076.*freq)+3.5*arctan((freq./7500).^2);
