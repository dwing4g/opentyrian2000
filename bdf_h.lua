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
local code, width, height, offsetX, offsetY, dataLine, data = 0, 0, 0, 0, 0, 0, {}
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
			-- w = tonumber(w)
			height = tonumber(h)
			offsetX = tonumber(x)
			offsetY = tonumber(y)
		elseif tag == "BITMAP" then
			dataLine = height
		elseif tag == "ENDCHAR" then
			local v = 0
			local b = 1
			local h = {}
			offsetY = 9 - offsetY - #data
			for j = 1, 11 do
				v = v + (data[j - offsetY] or 0) * b
				b = b * 0x800
				if b >= 0x100000000 then
					h[#h + 1] = string.format("%08X", v % 0x100000000)
					v = floor(v / 0x100000000)
					b = floor(b / 0x100000000)
				end
			end
			v = v + width * b
			h[#h + 1] = string.format("%08X", v)
			if code > 0 and code <= 0xffff then
				codes[code] = n
				f:write(string.format("\t0x%s%sULL,0x%s%sULL,//%04X,%d\n", h[2], h[1], h[4], h[3], code, n))
				n = n + 1
			end
			code, width, height, offsetX, offsetY, dataLine, data = 0, 0, 0, 0, 0, 0, {}
		elseif tag == "PIXEL_SIZE" then
			if args ~= "12" then
				error("ERROR(" .. i .. "): invalid: " .. line)
			end
		end
	else
		line = line:sub(1, 4)
		local v = tonumber(line, 16)
		if not v then error("ERROR(" .. i .. "): invalid hex: " .. line) end
		v = v * (2 ^ (16 - #line * 4))
		local v2, s = 0, 1
		for i = 1, 11 do -- only need 11 bits
			v = v + v
			if v >= 0x10000 then
				v = v - 0x10000
				v2 = v2 + s
			end
			s = s + s
		end
		v2 = v2 * (2 ^ offsetX)
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
