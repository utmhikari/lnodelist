# lnodelist

lua nodelist, a doubly linked list implementation written by lua C API

with a couple of common features and some syntax sugars in JS style

currently not thread safe nor asynchronous

glad if posting bugs and suggestions on issue TY XD~

## API

```c
// lnodelist.c
static const luaL_Reg lnodelist_f[] = {
  {"new", list_new},  // local l = list.new()
  {"size", list_size},  // list.size(l) or #l
  {"push", list_push},  // list.push(l, val)
  {"pushleft", list_pushleft},  // list.pushleft(l, val)
  {"pop", list_pop},  // list.pop(l) -> last val
  {"popleft", list_popleft},  // list.popleft(l) -> first val
  {"remove", list_remove},  // list.remove(l)
  {"removeleft", list_removeleft},  // list.removeleft(l)
  {"set", list_set},  // list.set(l, idx, val)
  {"get", list_get},  // list.get(l, idx) -> l[idx]
  {"insert", list_insert},  // list.insert(l, idx, val)
  {"delete", list_delete},  // list.delete(l, idx)
  {"reverse", list_reverse},  // list.reverse(l)
  {"clear", list_clear},  // list.clear(l)
  {"extend", list_extend},  // list.extend(l1, l2)
  {"slice", list_slice},  // list.slice(l [, start [, end]]) -> sl
  {"join", list_join},  // list.join(l, sep [,start [, end]])
  {"foreach", list_foreach},  // list.foreach(l, (val, idx) => {})
  {"map", list_map},  // list.map(l, (val, idx) => newVal) -> mapl
  {"some", list_some},  // list.some(l, (val, idx) => boolean)
  {"find", list_find},  // list.find(l, (val, idx) => trueVal)
  {"__len", list_size},  // same as list.size(l)
  {"__gc", list_clear},  // same as list.clear(l)
  {"__tostring", list_tostring},  // print list length and lua registry reference
  {NULL, NULL},
};
```

## Examples

- run `test.lua` for simple test example
- run `nodelist_vs_table.lua` for simple profiling example

### Basic Usage

```lua
local l1 = list.new()
for i = 1, 10, 1 do
  if i % 2 == 0 then
    list.push(l1, i);
  else
    list.pushleft(l1, i)
  end
end
print(tostring(l1))
-- NodeList <Length: 10, Reference: 3>
print(#l1 .. ": " .. list.join(l1, ", "))
-- 10: 9, 7, 5, 3, 1, 2, 4, 6, 8, 10
```

## TODOs

- better combination with lua core mechanism
- better solution on memory allocation
- more test cases & detailed bug/error handlers
- support lua table and other structs
- more syntax sugars
- thread safe support?
- async (coroutine) support?
- better solution on rawget(idx)?