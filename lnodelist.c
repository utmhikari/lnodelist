#include "lua.h"
#include "lauxlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifndef __LIST_NAME
#define __LIST_NAME "lnodelist"
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

/* check list on specific stack idx */
struct l_list *check_list(lua_State *L, int arg) {
  l_list *l = (l_list *)lua_touserdata(L, arg);
  luaL_argcheck(L, l != NULL, 1, "list expected");
  return l;
}

/* check validity of data type on specific idx */
int check_type(lua_State *L, int arg) {
  int type = lua_type(L, arg);
  if (type == LUA_TNONE) {
    luaL_error(L, "invalid data type!");
  }
  return type;
}

/* check validity of nodes */
void check_empty(lua_State *L, l_list *l) {
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- list is empty!", l->size);
  }
}

/* check validity of list index */
lua_Integer check_idx(lua_State *L, l_list *l, int arg) {
  lua_Integer idx = luaL_checkinteger(L, arg);
  if (idx <= 0 || idx > l->size) {
    luaL_error(L, "list size: %d --- index %d out of range!", l->size, idx);
  }
  return idx;
}

/* assign value to value pointer, may modify type */
void assign_val(lua_State *L, int arg, void **value, int *type) {
  switch (*type) {
    case LUA_TNUMBER:
      if (lua_isinteger(L, arg)) {
        *value = malloc(sizeof(lua_Integer));
        *(lua_Integer *)*value = lua_tointeger(L, arg);
        *type = LUA_TINTEGER;
      } else {
        *value = malloc(sizeof(lua_Number));
        *(lua_Number *)*value = lua_tonumber(L, arg);
      }
      break;
    case LUA_TSTRING:  // TODO: ...
      *value = malloc(sizeof(char) * lua_rawlen(L, arg));
      *(const char **)*value = lua_tostring(L, arg);
      break;
    case LUA_TBOOLEAN:
      *value = malloc(sizeof(int));
      *(int *)*value = lua_toboolean(L, arg);
      break;
    case LUA_TNIL:
      *value = NULL;
      break;
    default:  // store in registry index
      lua_pushvalue(L, arg);
      *value = malloc(sizeof(int));
      *(int *)*value = luaL_ref(L, LUA_REGISTRYINDEX);
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
    case LUA_TNIL:  // unsupported data type or nil
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

/* get the node at specific index, o(n/2) */
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
  l->size++;
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
  l->size++;
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
  l->size--;
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
  l->size--;
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
    l->size--;
  }
}

/* init a list */
static int list_new(lua_State *L) {
  l_list *l = (l_list *)lua_newuserdata(L, sizeof(l_list));
  l->size = 0;
  l->head = NULL;
  l->tail = l->head;
  // set metatable
  luaL_setmetatable(L, __LIST_NAME);
  return 1;
}

/* get size of the list */
static int list_size(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer size = (lua_Integer)(l->size);
  lua_settop(L, 0);
  lua_pushinteger(L, size);
  return 1;
}

/* push an node */
static int list_push(lua_State *L) {
  l_list *l = check_list(L, 1);
  int type = check_type(L, 2);
  // lazy-pushing node
  push_raw_node(l);
  assign_val(L, 2, &l->tail->value, &type);
  l->tail->type = type;
  return 1;
}

/* push an node to left */
static int list_pushleft(lua_State *L) {
  l_list *l = check_list(L, 1);
  int type = check_type(L, 2);
  pushleft_raw_node(l);
  assign_val(L, 2, &l->head->value, &type);
  l->head->type = type;
  return 1;
}

/* pop an node */
static int list_pop(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  push_val(L, l->tail);
  remove_tail(L, l);
  return 1;
}

/* pop an node at head */
static int list_popleft(lua_State *L) {
  l_list *l = check_list(L, 1);
  check_empty(L, l);
  push_val(L, l->head);
  remove_head(L, l);
  return 1;
}

/* set a list value */
static int list_set(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2);
  int type = check_type(L, 3);
  l_node *node = get_node(l, idx);
  free(node->value);
  assign_val(L, 3, &node->value, &type);
  return 1;
}

/* get node of list by index */
static int list_get(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2);
  // get the correct nodesor
  push_val(L, get_node(l, idx));
  return 1;
}

/* remove an node */
static int list_remove(lua_State *L) {
  l_list *l = check_list(L, 1);
  lua_Integer idx = check_idx(L, l, 2);
  remove_node(L, l, idx);
  return 1;
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
  return 1;
}

static int list_clear(lua_State *L) {
  l_list *l = check_list(L, 1);
  while (l->size > 0) {
    remove_tail(L, l);
  }
  return 1;
}

// static int list_pairs(lua_State *L) {

// }

static const luaL_Reg list_f[] = {
    {"new", list_new},
    {"size", list_size},
    {"push", list_push},
    {"pushleft", list_pushleft},
    {"pop", list_pop},
    {"popleft", list_popleft},
    {"set", list_set},
    {"get", list_get},
    {"remove", list_remove},
    {"reverse", list_reverse},
    {"clear", list_clear},
    // {"concat", list_concat},
    // {"extend", list_extend},
    // {"__pairs", list_pairs},
    {NULL, NULL},
};

LUAMOD_API int luaopen_nodelist(lua_State *L) {
  lua_newtable(L);
  luaL_setfuncs(L, list_f, 0);
  return 1;
}
