function [ lsb, msb ] = split( b2 )
% Splits a 2 byte number into its lsb and msb, little endian style
lsb = bitand(b2, 255, 'uint16');
msb = bitshift(b2, -8, 'uint16');

end

