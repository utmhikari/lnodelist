#include "lua.h"
#include "lauxlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifndef LUA_NODELIST
#define LUA_NODELIST "lnodelist"
#define LUA_TINTEGER 11
#endif

/* struct of node */
typedef struct l_node {
  int type;
  void *value;
  struct l_node *prev;
  struct l_node *next;
} l_node;

/* struct of list */
typedef struct l_list {
  lua_Integer size;
  struct l_node *head;
  struct l_node *tail;
} l_list;

/* push a new empty list on lua stack */
void new_list(lua_State *L) {
  l_list *l = (l_list *)lua_newuserdata(L, sizeof(l_list));
  l->size = 0;
  l->head = NULL;
  l->tail = l->head;
  luaL_setmetatable(L, LUA_NODELIST);
}

/* check list on specific stack idx */
struct l_list *check_list(lua_State *L, int arg) {
  l_list *l = (l_list *)lua_touserdata(L, arg);
  luaL_argcheck(L, l != NULL, 1, "list expected");
  return l;
}

/* check validity of nodes */
void check_empty(lua_State *L, l_list *l) {
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- list is empty!", l->size);
  }
}

/* check validity of list index */
lua_Integer check_idx(lua_State *L, l_list *l, int arg, lua_Integer opt) {
  lua_Integer i = luaL_optinteger(L, arg, opt);
  lua_Integer idx = i;
  if (i < 0) {
    idx = l->size + 1 + i;  // support idx below zero
  }
  if (idx <= 0 || idx > l->size) {
    luaL_error(L, "list size: %d --- index %d out of range!", l->size, i);
  }
  return idx;
}

/* assign value to value pointer, may modify type */
void assign_val(lua_State *L, l_node* node, int arg) {
  int type = lua_type(L, arg);
  switch (type) {
    case LUA_TNUMBER:
      if (lua_isinteger(L, arg)) {
        node->value = malloc(sizeof(lua_Integer));
        *(lua_Integer *)node->value = lua_tointeger(L, arg);
        node->type = LUA_TINTEGER;
      } else {
        node->value = malloc(sizeof(lua_Number));
        *(lua_Number *)node->value = lua_tonumber(L, arg);
        node->type = LUA_TNUMBER;
      }
      break;
    case LUA_TSTRING:  // TODO: ...
      node->value = malloc(sizeof(char) * lua_rawlen(L, arg));
      *(const char **)node->value = lua_tostring(L, arg);
      node->type = LUA_TSTRING;
      break;
    case LUA_TBOOLEAN:
      node->value = malloc(sizeof(int));
      *(int *)node->value = lua_toboolean(L, arg);
      node->type = LUA_TBOOLEAN;
      break;
    case LUA_TNIL:
    case LUA_TNONE:
      node->value = NULL;
      node->type = type;
      break;
    default:  // store in registry index
      lua_pushvalue(L, arg);
      node->value = malloc(sizeof(int));
      *(int *)node->value = luaL_ref(L, LUA_REGISTRYINDEX);
      node->type = type;
      break;
  }
}

/* get value of node and push to lua stack */
void push_val(lua_State *L, l_node *node) {
  switch (node->type) {
    case LUA_TINTEGER:
      lua_pushinteger(L, *(lua_Integer *)(node->value));
      break;
    case LUA_TNUMBER:
      lua_pushnumber(L, *(lua_Number *)(node->value));
      break;
    case LUA_TSTRING:
      lua_pushstring(L, *(const char **)(node->value));
      break;
    case LUA_TBOOLEAN:
      lua_pushstring(L, (int *)node->value ? "true" : "false");
      break;
    case LUA_TNIL:  // nil or none
    case LUA_TNONE:
      lua_pushnil(L);
      break;
    default:  // get from registry index
      lua_rawgeti(L, LUA_REGISTRYINDEX, *(int *)node->value);
      break;
  }
}

/* free a value, unref if neccessary */
void free_val(lua_State *L, l_node *node) {
  switch (node->type) {
    case LUA_TINTEGER:
    case LUA_TNUMBER: 
    case LUA_TSTRING:
    case LUA_TBOOLEAN:
    case LUA_TNIL:
      break;
    default:  // unref value
      luaL_unref(L, LUA_REGISTRYINDEX, *(int *)node->value);
      break;
  }
  free(node->value);
}

/* get the node at specific index */
l_node* get_node(l_list *l, lua_Integer idx) {
  l_node *node;
  if (idx == 1) {
    node = l->head;
  } else if (idx == l->size) {
    node = l->tail;
  } else if (idx - 1 <= l->size - idx){
    node = l->head;
    for (lua_Integer i = idx; i > 1; --i) {
      node = node->next;
    }
  } else {
    node = l->tail;
    for (lua_Integer i = l->size; i > idx; --i) {
      node = node->prev;
    }
  }
  return node;
}

/* push a node without value and type */
void push_raw_node(l_list *l) {
  if (l->head == NULL) {
    l->head = malloc(sizeof(l_node));
    l->tail = l->head;
    l->tail->prev = NULL;
  } else {
    l->tail->next = malloc(sizeof(l_node));
    l->tail->next->prev = l->tail;
    l->tail = l->tail->next;
  }
  l->tail->next = NULL;
  l->tail->value = NULL;
  ++l->size;
}

/* push a node to head without value and type */
void pushleft_raw_node(l_list *l) {
  if (l->head == NULL) {
    l->head = malloc(sizeof(l_node));
    l->tail = l->head;
    l->head->next = NULL;
  } else {
    l_node *head = malloc(sizeof(l_node));
    head->next = l->head;
    l->head->prev = head;
    l->head = head;
    head = NULL;
  }
  l->head->prev = NULL;
  l->head->value = NULL;
  ++l->size;
}

/* remove head */
void remove_head(lua_State *L, l_list *l) {
  l_node *head = l->head;
  l->head = l->head->next;
  if (l->head == NULL) {
    l->tail = NULL;
  } else {
    l->head->prev = NULL;
  }
  free_val(L, head);
  head->next = NULL;
  free(head);
  head = NULL;
  --l->size;
}

/* remove tail */
void remove_tail(lua_State *L, l_list *l) {
  l_node *tail = l->tail;
  l->tail = l->tail->prev;
  if (l->tail == NULL) {
    l->head = NULL;
  } else {
    l->tail->next = NULL;
  }
  free_val(L, tail);
  tail->prev = NULL;
  free(tail);
  tail = NULL;
  --l->size;
}

/* remove a node at sprcific index */
void remove_node(lua_State *L, l_list *l, lua_Integer idx) {
  l_node *node = get_node(l, idx);
  if (node->prev == NULL) {
    remove_head(L, l);
  } else if (node->next == NULL) {
    remove_tail(L, l);
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    free_val(L, node);
    free(node);
    node = NULL;
    --l->size;
  }
}

/* init a list */
static int list_new(lua_State *L) {
  new_list(L);
  return 1;
}

/* get size of the list */
static int list_size(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_pushinteger(L, l->size);
  return 1;
}

/* push a node */
static int list_push(lua_State *L) {
  l_list *l = check_list(L, 1);
  push_raw_node(l);
  assign_val(L, l->tail, 2);
  return 0;
}

/* push a node to left */
static int list_pushleft(lua_State *L) {
  l_list *l = check_list(L, 1);
  pushleft_raw_node(l);
  assign_val(L, l->head, 2);
  return 0;
}

/* pop a node */
static int list_pop(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  push_val(L, l->tail);
  remove_tail(L, l);
  return 1;
}

/* pop a node at head */
static int list_popleft(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  push_val(L, l->head);
  remove_head(L, l);
  return 1;
}

/* remove a node from tail */
static int list_remove(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  remove_tail(L, l);
  return 0;
}

/* remove a node from tail */
static int list_removeleft(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  remove_head(L, l);
  return 0;
}

/* set a list value */
static int list_set(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2, 0);
  l_node *node = get_node(l, idx);
  free_val(L, node);
  assign_val(L, node, 3);
  return 0;
}

/* get node of list by index */
static int list_get(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2, 0);
  // get the correct nodesor
  push_val(L, get_node(l, idx));
  return 1;
}

/* insert a node to specific index */
static int list_insert(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = luaL_checkinteger(L, 2);
  if (idx <= 0 || idx > l->size + 1) {  // insert can be at l->size + 1!
    luaL_error(L, "list size: %d --- index %d out of range!", l->size, idx);
  }
  l_node *node;
  if (idx == 1) {
    pushleft_raw_node(l);
    node = l->head;
  } else if (idx == l->size + 1) {
    push_raw_node(l);
    node = l->tail;
  } else {
    l_node *prev = get_node(l, idx - 1);
    l_node *next = prev->next;
    node = malloc(sizeof(l_node));
    prev->next = node;
    node->prev = prev;
    next->prev = node;
    node->next = next;
    prev = NULL;
    next = NULL;
    ++l->size;
  }
  assign_val(L, node, 3);
  node = NULL;
  return 0;
}

/* delete a specific node */
static int list_delete(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2, 0);
  remove_node(L, l, idx);
  return 0;
}

/* reverse list */
static int list_reverse(lua_State *L) {
  l_list *l = check_list(L, 1);
  if (l->size <= 1) {
    return 0;  // no need to reverse
  }
  l->head = l->tail;
  l_node *temp = l->tail->prev;
  while (temp != NULL) {
    l->tail->prev = l->tail->next;
    l->tail->next = temp;
    l->tail = temp;
    temp = temp->prev;
  }
  l->tail->prev = l->tail->next;
  l->tail->next = NULL;
  return 0;
}

/* clear the whole list */
static int list_clear(lua_State *L) {
  l_list *l = check_list(L, 1);
  while (l->size > 0) {
    remove_tail(L, l);
  }
  return 0;
}

/* extend list l1 by another list l2 */
static int list_extend(lua_State *L) {
  l_list *l1 = check_list(L, 1);
  l_list *l2 = check_list(L, 2);
  l_node *node = l2->head;
  while (node != NULL) {
    push_raw_node(l1);
    push_val(L, node);
    assign_val(L, l1->tail, -1);
    lua_pop(L, 1);
    node = node->next;
  }
  return 0;
}

/* return a new sliced list */
static int list_slice(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer start = check_idx(L, l, 2, 1);
  lua_Integer end = check_idx(L, l, 3, l->size);
  if (start > end) {
    luaL_error(L, "Unable to slice list with start %d larger than end %d!", start, end);
  }
  l_node *node = get_node(l, start);
  new_list(L);
  l_list *sl = check_list(L, -1);
  for (lua_Integer i = end; i >= start; --i) {
    push_raw_node(sl);
    push_val(L, node);
    assign_val(L, sl->tail, -1);
    lua_pop(L, 1);
    node = node->next;
  }
  return 1;
}

/* join parts of list to string */
static int list_join(lua_State *L) {
  l_list *l = check_list(L, 1);
  if (l->size == 0) {
    lua_pushstring(L, "");
    return 1;
  }
  size_t lsep;
  const char *sep = luaL_optlstring(L, 2, "", &lsep);
  lua_Integer start = check_idx(L, l, 3, 1);
  lua_Integer end = check_idx(L, l, 4, l->size);
  if (start > end) {
    luaL_error(L, "Unable to join list with start %d larger than end %d!", start, end);
  }
  l_node *node = get_node(l, start);
  luaL_Buffer sb;
  luaL_buffinit(L, &sb);
  while (start < end) {
    push_val(L, node);
    luaL_addvalue(&sb);
    luaL_addlstring(&sb, sep, lsep);
    node = node->next;
    ++start;
  }
  if (start == end) {
    push_val(L, node);
    luaL_addvalue(&sb);
  }
  luaL_pushresult(&sb);
  return 1;
}

/* foreach loop for stateful iteration */
static int list_foreach(lua_State *L) {
  l_list *l = check_list(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  l_node *node = l->head;
  lua_Integer idx = 0;
  while (node != NULL) {
    lua_pushvalue(L, 2);
    push_val(L, node);
    lua_pushinteger(L, ++idx);
    lua_call(L, 2, 0);
    node = node->next;
  }
  return 0;
}

/* get a mapped list */
static int list_map(lua_State *L) {
  l_list *l = check_list(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  new_list(L);
  l_list *ml = check_list(L, -1);
  l_node *node = l->head;
  lua_Integer idx = 0;
  while (node != NULL) {
    lua_pushvalue(L, 2);
    push_val(L, node);
    lua_pushinteger(L, ++idx);
    lua_call(L, 2, 1);
    push_raw_node(ml);
    assign_val(L, ml->tail, -1);
    lua_pop(L, 1);
    node = node->next;
  }
  return 1;
}

/* some: find if the list has value fitting the condition */
static int list_some(lua_State *L) {
  l_list *l = check_list(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  l_node *node = l->head;
  lua_Integer idx = 0;
  while (node != NULL) {
    lua_pushvalue(L, 2);
    push_val(L, node);
    lua_pushinteger(L, ++idx);
    lua_call(L, 2, 1);
    if (lua_toboolean(L, -1)) {
      lua_pushboolean(L, 1);
      return 1;
    }
    lua_pop(L, 1);
    node = node->next;
  }
  lua_pushboolean(L, 0);
  return 1;
}

/* find: find the first element that fits the condition */
static int list_find(lua_State *L) {
  l_list *l = check_list(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  l_node *node = l->head;
  lua_Integer idx = 0;
  while (node != NULL) {
    lua_pushvalue(L, 2);
    push_val(L, node);
    lua_pushinteger(L, ++idx);
    lua_call(L, 2, 1);
    if (lua_toboolean(L, -1)) {
      push_val(L, node);
      return 1;
    }
    lua_pop(L, 1);
    node = node->next;
  }
  lua_pushnil(L);
  return 1;
}

static int list_tostring(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_pushfstring(L, "NodeList <Length: %d, Reference: %d>", l->size, luaL_ref(L, LUA_REGISTRYINDEX));
  return 1;
}

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


LUAMOD_API int luaopen_lnodelist(lua_State *L) {
  luaL_newmetatable(L, LUA_NODELIST);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, lnodelist_f, 0);
  return 1;
}
