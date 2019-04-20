local list = require "lnodelist"

local function sep(title)
  title = title or "Separator"
  print("\n-----------------------  " .. tostring(title) .. " -----------------------\n")
end

sep("push, pop, remove, insert, delete, join, reverse, set, get, __len, __tostring")

local l1 = list.new()
for i = 1, 10, 1 do
  if i % 2 == 0 then
    list.push(l1, i);
  else
    list.insert(l1, #l1 + 1, i)
  end
end
print(#l1 .. ": " .. list.join(l1, ", "))
for i = 1, 5, 1 do
  if i % 2 == 0 then
    list.popleft(l1)
  else
    list.removeleft(l1)
  end
end
print(#l1 .. ": " .. list.join(l1, ", "))
print(tostring(l1))
print(#l1 .. ": " .. list.join(l1, ", "))
for i = 1, 10, 1 do
  if i % 2 == 0 then
    list.pushleft(l1, i * 10)
  else
    list.insert(l1, 1, i * 10)
  end
end
print(#l1 .. ": " .. list.join(l1, ", "))
for i = 1, 5, 1 do
  if i % 2 == 0 then
    list.remove(l1)
  else
    list.pop(l1)
  end
end
print(list.size(l1) .. ": " .. list.join(l1, ", "))
for i = 1, 3, 1 do
  list.delete(l1, i * 2)
end
print(#l1 .. ": " .. list.join(l1, ", "))
list.set(l1, 2, "haha")
print(#l1 .. ": " .. list.join(l1, ", "))
print(list.get(l1, 4))

sep("extend, slice, clear")

local l2 = list.new()
for i = 1, 5, 1 do
  list.push(l2, i)
end
list.extend(l2, l1)
print(#l2 .. ": " .. list.join(l2, ", "))
local l2_slice = list.slice(l2, 3, 8)
print(#l2_slice .. ": " .. list.join(l2_slice, ", "))
list.clear(l2_slice)
print(#l2_slice .. ": " .. list.join(l2_slice, ", "))

sep("foreach, map, some, find")

local l3 = list.new()
for i = 1, 5, 1 do
  list.push(l3, i)
end
list.foreach(l3, function(val, idx)
  print(idx .. " -- " .. val)
end)
local l4 = list.map(l3, function(val, idx)
  return val + idx * 10
end)
print(list.join(l4, ", "))
print(list.some(l4, function(val, _)
  return val > 50
end))
print(list.find(l4, function(val, _)
  return val > 50
end))
