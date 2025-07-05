-- luajit shape_dec.lua data/shapes).dat shapes).tga
-- luajit shape_dec.lua data/shapesW.dat shapesW.tga
-- luajit shape_dec.lua data/shapesX.dat shapesX.tga
-- luajit shape_dec.lua data/shapesY.dat shapesY.tga
-- luajit shape_dec.lua data/shapesZ.dat shapesZ.tga
--[[ tga header
00: 00
01: 01    (0:no pal; 1:has pal)
02: 01    (1:8-bit pal; 2:24/32-bit color; 3:8-bit gray; 9~11:RLE)
03: 00 00 (pal from: 0)
05: 00 01 (pal to: 256)
07: 18    (pal: 24-bit)
08: 00 00
0A: 00 00
0C: ww ww (width)
0E: hh hh (height)
10: 08    (pixel bits)
11: 20    (00:from bottom; 20:from top)
12: [3*256] (pal:BGRBGR...)

24*28 * 10*60 = 240*1680
--]]
local arg = arg
local floor = math.floor
local string = string
local byte = string.byte
local char = string.char
local concat = table.concat
local io = io
local write = io.write

local function tranPal(c)
	local v = byte(c)
	v = v * 4 + floor(v / 16)
	return char(v)
end

local palIdx = 0
local f = io.open("data/palette.dat", "rb")
f:seek("set", palIdx * 0x300)
local s = f:read(0x300)
f:close()
local t = {}
for i = 1, 0x300, 3 do
	t[i  ] = tranPal(s:sub(i+2)) -- B
	t[i+1] = tranPal(s:sub(i+1)) -- G
	t[i+2] = tranPal(s:sub(i  )) -- R
end
local pal = concat(t)

f = io.open(arg[1], "rb")
s = f:read "*a"
f:close()

f = io.open(arg[2], "wb")
f:write "\x00\x01\x01\x00\x00\x00\x01\x18\x00\x00\x00\x00"
f:write(char(       240 % 256 ))
f:write(char(floor( 240 / 256)))
f:write(char(      1680 % 256 ))
f:write(char(floor(1680 / 256)))
f:write "\x08\x20"
f:write(pal)
local p = 0
for y = 1, 60 do
	t = {}
	for x = 1, 10 do
		p = p + 1
		if s:byte(p) == 0 then
			t[x] = s:sub(p+1, p+24*28)
			p = p + 24*28
		else
			t[x] = string.rep("\x00", 24*28)
		end
	end
	for yy = 0, 28-1 do
		for x = 1, 10 do
			f:write(t[x]:sub(yy*24+1, yy*24+24))
		end
	end
end
f:close()

print "DONE!"
