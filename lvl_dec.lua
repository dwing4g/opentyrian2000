-- luajit lvl_dec.lua data/tyrian1.lvl > level1.txt
-- luajit lvl_dec.lua data/tyrian2.lvl > level2.txt
-- luajit lvl_dec.lua data/tyrian3.lvl > level3.txt
-- luajit lvl_dec.lua data/tyrian4.lvl > level4.txt
-- luajit lvl_dec.lua data/tyrian5.lvl > level5.txt
--[[ map tiles
tile_size : 24 * 28
tile_count: 14/15 * 600 (360 * 16800)
screen_tiles: 15 * 8
--]]
local error = error
local arg = arg
local floor = math.floor
local string = string
local char = string.char
local format = string.format
local concat = table.concat
local io = io
local write = io.write

local f = io.open(arg[1], "rb")
if not f then
	error("ERROR: can not open file: " .. arg[1])
end
local s = f:read "*a"
f:close()

local pos, p = 0, 0
local var = {}

local function out0(...)
	write(format("%06X: ", p), format(...))
	p = pos
end

local function out1(...)
	write(format(...))
end

local function outln(...)
	write(format("%06X: ", p), format(...), "\n")
	p = pos
end

local function dumpbin(n)
	local t = {}
	for i = 1, n do
		t[i] = format("%02X", s:byte(pos + i))
	end
	pos = pos + n
	return concat(t, " ")
end

local function read1(varname)
	local a = s:byte(pos + 1)
	pos = pos + 1
	if varname then
		var[varname] = a
	end
	return a
end

local function read1s(varname)
	local a = s:byte(pos + 1)
	if a >= 0x80 then
		a = a - 0x100
	end
	pos = pos + 1
	if varname then
		var[varname] = a
	end
	return a
end

local function read2(varname)
	local a, b = s:byte(pos + 1, pos + 2)
	local v = a + b * 0x100
	pos = pos + 2
	if varname then
		var[varname] = v
	end
	return v
end

local function read2s(varname)
	local a, b = s:byte(pos + 1, pos + 2)
	local v = a + b * 0x100
	if v >= 0x8000 then
		v = v - 0x10000
	end
	pos = pos + 2
	if varname then
		var[varname] = v
	end
	return v
end

local function read2be(varname)
	local a, b = s:byte(pos + 1, pos + 2)
	local v = a * 0x100 + b
	pos = pos + 2
	if varname then
		var[varname] = v
	end
	return v
end

local function read4(varname)
	local a, b, c, d = s:byte(pos + 1, pos + 4)
	local v = a + b * 0x100 + c * 0x10000 + d * 0x1000000
	pos = pos + 4
	if varname then
		var[varname] = v
	end
	return v
end

outln("level_count[2]: %d", read2("n") / 2)
for i = 0, var.n / 2 - 1 do
	outln("  level_%02d_offset[4]: %06X %06X", i+1, read4("offsetA" .. i), read4("offsetB" .. i))
end
for i = 0, var.n / 2 - 1 do
	pos = var["offsetA" .. i]
	outln("level_%02d", i+1)
	outln("  char_map_file  [1]: %s", char(read1()))
	outln("  char_shape_file[1]: %s -- shapes%s.dat", char(read1("sn")), char(var.sn))
	outln("  map_x[2*3]: %d,%d,%d", read2(), read2(), read2())
	outln("  enemy_count[2]: %d", read2("en"))
	out0 ("  enemy_types[2*%d]:", var.en)
	for j = 1, var.en do
		out1 (" %d", read2())
	end
	write "\n"
	outln("  event_count[2]: %d", read2("en"))
	outln("            time[2] type[1] dat[2] dat2[2] dat3[1] dat5[1] dat6[1] dat4[1]")
	for j = 0, var.en - 1 do
		outln("    event%04d:%5d %7d %6d %7d %7d %7d %7d %7d",
			j, read2(), read1(), read2s(), read2s(), read1s(), read1s(), read1s(), read1())
	end
	pos = var["offsetB" .. i]
	for j = 0, 0x17 do
		out0("  shape_table_%d_%d:", floor(j / 8), j % 8) -- shapes%c.dat
		for k = 0, 15 do
			out1(" %3d", read2be()-1)
		end
		write "\n"
	end
	outln("  map_buf_1:")
	for j = 0, 299 do
		outln("    %s", dumpbin(14))
	end
	outln("  map_buf_2:")
	for j = 0, 599 do
		outln("    %s", dumpbin(14))
	end
	outln("  map_buf_1:")
	for j = 0, 599 do
		outln("    %s", dumpbin(15))
	end
end
outln("===")
