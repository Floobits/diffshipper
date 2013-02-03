#!/usr/bin/env lua

-- From http://lua-users.org/lists/lua-l/2008-11/msg00453.html
-- Written by Mildred Ki'Lya
-- Invocation: lua bin2c.lua c_symbol [static] <input_file.lua >output_file.c

-- lua bin2c.lua lua_diff_match_patch_str < src/lua/diff_match_patch.lua > src/dmp_lua_str.h

local name, static = ...

name = name or "stdin"
static = static or ""

local out = { static, " ", "char ", name, "[] = {\n  " }
if static == '' then
  out[2] = ''
end

local len = 0
local c = io.read(1)
local l = 2;
while c do
  len = len + 1
  if l + 7 > 80 then
    out[#out] = ",\n  ";
    l = 2
  end
  out[#out+1] = ("0x%02x"):format(c:byte())
  out[#out+1] = ", "
  l = l + 6
  c = io.read(1)
end
out[#out] = " };\n\n";
out[#out+1] = "int "
out[#out+1] = name
out[#out+1] = "_size = "
out[#out+1] = tostring(len)
out[#out+1] = ";\n\n"

io.write(table.concat(out))
