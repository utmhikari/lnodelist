local list = require "lnodelist"

local l = list.new()
local t = {}

local timer = os.clock()
for i = 1, 10000000, 1 do
    list.push(l, i)
end
print("nodelist push: " .. os.clock() - timer)

timer = os.clock()
for i = 1, 10000000, 1 do
    table.insert(t, i)
end
print("table insert: " .. os.clock() - timer)

timer = os.clock()
for i = 1, 10000000, 1 do
    list.remove(l)
end
print("nodelist remove: " .. os.clock() - timer)

timer = os.clock()
for i = 1, 10000000, 1 do
    table.remove(t)
end
print("table remove: " .. os.clock() - timer)