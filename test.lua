local list = require "lnodelist"

local l = list.new()
local startl = os.clock()
for i = 1, 100000, 1 do
  list.push(l, tostring(i))
end
print(os.clock() - startl)

local t = {}
local startt = os.clock()
for i = 1, 100000, 1 do
  table.insert(t, tostring(i))
end
print(os.clock() - startt)









