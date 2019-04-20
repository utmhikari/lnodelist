local list = require "lnodelist"

local l = list.new()
local startl = os.clock()
for i = 1, 10, 1 do
  list.push(l, i * 3)
end
local l2 = list.new()
for i = 10, 100, 10 do
  list.push(l2, i)
end
list.extend(l, l2)
print(list.size(l))
list.foreach(l, function (val, idx)
  print(idx .. ": " .. val)
end)












