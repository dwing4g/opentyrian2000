-- luajit bg_dec.lua <1..5> <1..> <1..3>
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

24*28 * 15*600 = 360*16800
--]]
local arg = arg
local floor = math.floor
local string = string
local char = string.char
local concat = table.concat
local io = io
local write = io.write
local error = error
local tonumber = tonumber

arg[1] = arg[1] or "1"
arg[2] = arg[2] or "1"
arg[3] = arg[3] or "1"

local function read2(s, pos)
	local a, b = s:byte(pos + 1, pos + 2)
	return a + b * 0x100
end

local function read2be(s, pos)
	local a, b = s:byte(pos + 1, pos + 2)
	return a * 0x100 + b
end

local function read4(s, pos)
	local a, b, c, d = s:byte(pos + 1, pos + 4)
	return a + b * 0x100 + c * 0x10000 + d * 0x1000000
end

local function tranPal(c)
	local v = c:byte(1)
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

f = io.open("data/tyrian" .. arg[1] .. ".lvl", "rb")
s = f:read "*a"
f:close()
local levels = read2(s, 0)
local level = tonumber(arg[2])
if level > levels then
	error("ERROR: level overflow: >" .. levels)
end
level = level - 1
local layer = tonumber(arg[3])
if layer > 3 then
	error("ERROR: layer overflow: >3")
end
layer = layer - 1

local offset = read4(s, 2 + 8*level)
f = io.open("data/shapes" .. s:sub(offset+2,offset+2) .. ".dat", "rb")
local ss = f:read "*a"
f:close()
local shapes = {}
local p = 0
for i = 0, 599 do
	p = p + 1
	if ss:byte(p) == 0 then
		shapes[i] = ss:sub(p+1, p+24*28)
		p = p + 24*28
	else
		shapes[i] = string.rep("\x00", 24*28)
	end
end

offset = read4(s, 2 + 8*level + 4)
local indexes = {}
for i = 0, 127 do
	indexes[i] = read2be(s, offset + layer*256 + i*2) - 1
end
offset = offset + 3*256
local w, h
if layer == 0 then
	w = 14
	h = 300
	s = s:sub(offset+1, offset+w*h)
elseif layer == 1 then
	w = 14
	h = 600
	offset = offset + 14*300
	s = s:sub(offset+1, offset+w*h)
elseif layer == 2 then
	w = 15
	h = 600
	offset = offset + 14*300 + 14*600
	s = s:sub(offset+1, offset+w*h)
end

f = io.open("bg_" .. arg[1] .. "_" .. arg[2] .. "_" ..arg[3] .. ".tga", "wb")
f:write "\x00\x01\x01\x00\x00\x00\x01\x18\x00\x00\x00\x00"
f:write(char(      w*24 % 256 ))
f:write(char(floor(w*24 / 256)))
f:write(char(      h*28 % 256 ))
f:write(char(floor(h*28 / 256)))
f:write "\x08\x20"
f:write(pal)
for y = 0, h-1 do
	for yy = 0, 27 do
		for x = 0, w-1 do
			local i = indexes[s:byte(1 + w * y + x)]
			local shape = shapes[i]
			if not shape then error("ERROR: no shape: " .. i) end
			f:write(shape:sub(yy*24+1, yy*24+24))
		end
	end
end
f:close()

print "DONE!"
