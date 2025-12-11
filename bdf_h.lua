--[[
PIXEL_SIZE 12
FONT_ASCENT 10
FONT_DESCENT 2
{
STARTCHAR u0020
ENCODING 32
DWIDTH 6 0 // 4 0 | 8 0 | 12 0
BBX 6 12 0 -2 // 4 14 0 -3 | 8 14 0 -3 | 12 12 0 -2 | 12 14 0 -3 | 12 24 0 -8 | 包围盒宽高+包围盒左下角对基准原点的偏移(正方向:↑→)
BITMAP
0000
0180 // 每字符从高位开始4bit
1FE0
....
ENDCHAR
}*
--]]
local floor = math.floor
local codes = {}
local code, width, height, dataLine, data = 0, 0, 0, 0, {}
local f = io.open(arg[2] or "src/font_chs_data.h", "wb")
f:write "#include <stdint.h>\n\n"
f:write "__declspec(align(16))\n"
f:write "uint64_t font_chs_data[] = {\n"
local i, n = 0, 0
for line in io.lines(arg[1] or "fusion-pixel-12px-monospaced-zh_hans.bdf") do -- https://github.com/TakWolf/fusion-pixel-font
	i = i + 1
	if dataLine == 0 then
		local tag, args = line:match "^([%w_]+)%s*(.*)$"
		if tag == "ENCODING" then
			code = tonumber(args)
		elseif tag == "DWIDTH" then
			width = tonumber(args:match "^%d+")
			if not width then error("ERROR(" .. i .. "): invalid: " .. line) end
		elseif tag == "BBX" then
			local w, h, x, y = args:match "^(%d+) (%d+) ([%d%-]+) ([%d%-]+)$"
			if not w then error("ERROR(" .. i .. "): invalid: " .. line) end
			w = tonumber(w)
			h = tonumber(h)
			x = tonumber(x)
			y = tonumber(y)
			if w ~= width then error("ERROR(" .. i .. "): invalid w: " .. line) end
			if x ~= 0 then error("ERROR(" .. i .. "): invalid x: " .. line) end
			if not (h == 12 and y == -2 or h == 14 and y == -3 or h == 24 and y == -8) then error("ERROR(" .. i .. "): invalid h&y: " .. line) end
			height = h
		elseif tag == "BITMAP" then
			dataLine = height
		elseif tag == "ENDCHAR" then
			local v = 0
			local b = 1
			local h = {}
			for j = height == 12 and 2 or 3, height == 12 and 12 or 13 do
				v = v + data[j] * b
				b = b * 0x800
				if b >= 0x100000000 then
					h[#h + 1] = string.format("%08X", v % 0x100000000)
					v = floor(v / 0x100000000)
					b = floor(b / 0x100000000)
				end
			end
			v = v + width * b
			h[#h + 1] = string.format("%08X", v)
			if code <= 0xffff then
				codes[code] = n
				f:write(string.format("\t0x%s%sULL,0x%s%sULL,//%04X,%d\n", h[2], h[1], h[4], h[3], code, n))
				n = n + 1
			end
			code, width, height, dataLine, data = 0, 0, 0, 0, {}
		elseif tag == "PIXEL_SIZE" then
			if args ~= "12" then
				error("ERROR(" .. i .. "): invalid: " .. line)
			end
		end
	else
		local v = tonumber(line, 16)
		local v2 = 0
		for i = 1, #line * 4 do
			v2 = v2 * 2 + v % 2
			v = floor(v / 2)
		end
		data[#data + 1] = v2 % 0x800 -- only need 11 bits, from low(left) to high(right)
		dataLine = dataLine - 1
	end
end
f:write "};\n"
f:write "uint16_t font_chs_index[0x10000] = {\n"
for i = 0, 0xffff do
	if i % 16 == 0 then
		f:write(string.format("\t/*%04X*/", i))
	end
	f:write(string.format("%d,", codes[i] or -1))
	if i % 16 == 15 then
		f:write "\n"
	end
end
f:write "};\n"
f:close()
print "DONE!"
