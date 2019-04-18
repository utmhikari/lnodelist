# lua-list

list implementation of lua

## Usage

```lua
local list = require "list"
local l = list.new("n")  -- list<number>
list.add(l, 1)
list.add(l, 3)
print(list.get(l, 1))
print(list.size(l))
```
