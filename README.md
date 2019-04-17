# lnodelist

doubly linked list implementation on lua c api

## Usage

```lua
local list = require "list"
local l = list.new()
list.push(l, 1)
list.push(l, 3)
list.reverse(l)
print(list.get(l, 1))
print(list.size(l))
```
