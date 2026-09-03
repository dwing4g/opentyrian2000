local f = io.popen("dir /b/s translation\\*.txt")
local s = f:read "*a"
f:close()

local t = {}
for fn in s:gmatch "%C+" do
	local k
	print("INFO: checking " .. fn .. " ...")
	for line in io.lines(fn) do
		if line == "" then
			k = nil
		elseif not k then
			k = line
			if k:find "[^\x20-\x7e]" then
				print("ERROR: invalid key line: " .. k)
			end
		else
			if t[k] then
				if t[k] ~= line then
					print(k)
					print(t[k])
					print(line)
					print "---"
				end
			else
				t[k] = line
			end
			k = nil
		end
	end
	if k then
		print("ERROR: no value for last line: " .. fn)
	end
end
print "==="
