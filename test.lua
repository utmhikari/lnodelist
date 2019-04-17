local list = require "list"

local l = list.new()
local l1 = list.new()

-- l

for i = 1, 8, 1 do
  list.push(l, i)
end

list.reverse(l);
-- list.reverse(l);

list.remove(l, 3)

list.remove(l, 1)

list.pop(l)
list.popleft(l)
for i = 1, 8, 1 do
  print(list.get(l, i))
end

-- for i = 10, 2, -1 do
--   list.pop(l)
--   print(tostring(list.size(l)) .. " -- " .. tostring(list.get(l, i - 1)))
-- end





