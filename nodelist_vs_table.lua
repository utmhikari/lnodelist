local list = require "lnodelist"

local l = list.new()
local t = {}

local timer = os.clock()
for i = 1, 10000000, 1 do
    list.push(l, i)
end
print("nodelist push: " .. os.clock() - timer)  -- beats only table.insert()

timer = os.clock()
for i = 1, 10000000, 1 do
    t[i] = i  -- so fast!
end
print("table insert: " .. os.clock() - timer)

timer = os.clock()
for i = 1, 10000000, 1 do
    list.pop(l)
end
print("nodelist remove: " .. os.clock() - timer)  -- beats only table.remove()

timer = os.clock()
for i = 1, 10000000, 1 do
    t[i] = nil  -- so fast!
end
print("table remove: " .. os.clock() - timer)

-- pushleft? popleft?