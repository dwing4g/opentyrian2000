-- luajit cube_txt.lua cubetxt1.dat.txt cubetxt1.txt
-- luajit cube_txt.lua cubetxt2.dat.txt cubetxt2.txt
-- luajit cube_txt.lua cubetxt3.dat.txt cubetxt3.txt
-- luajit cube_txt.lua cubetxt4.dat.txt cubetxt4.txt
-- luajit cube_txt.lua cubetxt5.dat.txt cubetxt5.txt

local f = io.open(arg[2], "wb")
local s = 0
local t = {}
for line in io.lines(arg[1]) do
	if line:sub(1,1) == "*" then
		if t[1] then
			f:write(table.concat(t, " "), "\n")
			t = {}
		end
		s = 1
		f:write("\n", line:gsub("^%*", "# "), "\n\n")
	else
		line = line:gsub("~", "*")
		if s == 1 then
			s = 2
			f:write("- Title: ", line, "\n")
		elseif s == 2 then
			s = 3
			f:write("- Header: ", line, "\n- Message:\n")
		elseif s == 3 then
			if line ~= "" then
				t[#t + 1] = line
			else
				f:write(table.concat(t, " "), "\n")
				t = {}
			end
		end
	end
end
if t[1] then
	f:write(table.concat(t, " "), "\n")
	t = {}
end
f:close()
print "DONE!"
