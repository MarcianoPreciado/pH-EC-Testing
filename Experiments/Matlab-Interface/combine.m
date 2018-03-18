function [ b2 ] = combine( lsb, msb )
% Combines the lsb and msb into a single byte, little endian style
lsb = uint16(lsb);
msb = uint16(msb);
b2 = uint16(bitor(lsb, bitshift(msb, 8, 'uint16')));

end

