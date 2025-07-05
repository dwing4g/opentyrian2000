local key = { 204, 129, 63, 255, 71, 19, 25, 62, 1, 99 }
local function dec(s)
	local n = #s
	local t = { s:byte(1, n) }
	if n > 0 then
		for i = n - 1, 0, -1 do
			t[i+1] = bit.bxor(t[i+1], key[i % #key + 1])
			if i == 0 then break end
			t[i+1] = bit.bxor(t[i+1], t[i])
		end
		for i, v in ipairs(t) do
			t[i] = string.char(t[i])
		end
		s = table.concat(t)
	end
	return s
end

local function decFH(fn, fon)
	local f = io.open(fn, "rb")
	local fo = io.open(fon, "wb")
	local a, b, c, d = f:read(4):byte(1, 4)
	local e = a + b*0x100 + c*0x10000 + d*0x1000000
	local p = 4
	while p < e do
		local n = f:read(1):byte(1)
		local s = dec(f:read(n))
		p = p + 1 + n
		fo:write(s, '\n')
	end
	fo:close()
	f:close()
end

local function decF(fn, fon)
	local f = io.open(fn, "rb")
	local fo = io.open(fon, "wb")
	while true do
		local n = f:read(1)
		if not n then break end
		local n = n:byte(1)
		local s = n > 0 and dec(f:read(n)) or ""
		fo:write(s, '\n')
	end
	fo:close()
	f:close()
end

decFH("data/tyrian.hdt", "tyrian.hdt.txt")
decF("data/tyrian.cdt", "tyrian.cdt.txt")

for i = 1, 5 do
	decF("data/levels" .. i .. ".dat", "levels" .. i .. ".dat.txt")
end
for i = 1, 5 do
	decF("data/cubetxt" .. i .. ".dat", "cubetxt" .. i .. ".dat.txt")
end

print "DONE!"
