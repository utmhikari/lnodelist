#include "lua.h"
#include "lauxlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifndef __LIST_NAME
#define __LIST_NAME "list"
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
  size_t size;
  struct l_node *head;
  struct l_node *tail;
} l_list;

/* check list on top of stack */
struct l_list *lua_checklist(lua_State *L, int arg) {
  l_list *l = (l_list *)lua_touserdata(L, arg);
  luaL_argcheck(L, l != NULL, 1, "list expected");
  return l;
}

/* get value and push to lua stack */
int lua_pushlistval(lua_State *L, l_node *node) {
  // get the value
  switch (node->type) {
    case LUA_TNUMBER:
      lua_pushnumber(L, *(lua_Number *)(node->value));
      break;
    default:
      lua_pushnil(L);
      return 0;
  }
  return 1;
}

/* assign value to value pointer, may modify type */
int assign_value(lua_State *L, int arg, void **value, int *type) {
  // check type
  switch (*type) {
    case LUA_TNUMBER:
      *value = malloc(sizeof(lua_Number));
      *(lua_Number *)*value = lua_tonumber(L, arg);
      break;
    default:
      return 0;  // unreachable
  }
  return 1;
}

/* get the node at specific index */
l_node* get_node(l_list *l, lua_Integer idx) {
  l_node *node;
  if (idx == 1) {
    node = l->head;
  } else if (idx == l->size) {
    node = l->tail;
  } else {
    node = l->head;
    for (int i = idx; i > 1; i--) {
      node = node->next;
    }
  }
  return node;
}

/* remove head */
void remove_head(l_list *l) {
  free(l->head->value);
  if (l->head->next == NULL) {  // only one node
    l->tail = NULL;
    free(l->head);
    l->tail = l->head;
  } else {
    l_node *head = l->head->next;
    head->prev = NULL;
    l->head->next = NULL;
    free(l->head);
    l->head = head;
    head = NULL;
  }
  l->size--;
}

/* remove tail */
void remove_tail(l_list *l) {
  free(l->tail->value);
  if (l->tail->prev == NULL) {  // only one node
    l->head = NULL;
    free(l->tail);
    l->tail = NULL;
  } else {
    l_node *tail = l->tail->prev;
    l->tail->prev = NULL;
    tail->next = NULL;
    free(l->tail);
    l->tail = tail;
    tail = NULL;
  }
  l->size--;
}

/* remove a node at sprcific index */
void remove_node(l_list *l, lua_Integer idx) {
  l_node *node = get_node(l, idx);
  if (node->prev == NULL) {
    remove_head(l);
  } else if (node->next == NULL) {
    remove_tail(l);
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    free(node->value);
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

/* push an node */
static int list_push(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  int type = lua_type(L, 2);
  if (type == LUA_TNONE) {
    luaL_error(L, "invalid data type!");
  }
  // lazy-pushing node
  if (l->head == NULL) {
    l->head = malloc(sizeof(l_node));
    l->tail = l->head;
    l->tail->prev = NULL;
  } else {
    l->tail->next = malloc(sizeof(l_node));
    l->tail->next->prev = l->tail;
    l->tail = l->tail->next;
  }
  assign_value(L, 2, &l->tail->value, &type);
  l->tail->next = NULL;
  l->tail->type = type;
  l->size++;
  return 1;
}

/* push an node to left */
static int list_pushleft(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  int type = lua_type(L, 2);
  if (type == LUA_TNONE) {
    luaL_error(L, "invalid data type!");
  }
  // lazy pushing node
  if (l->head == NULL) {
    l->head = malloc(sizeof(l_node));
    l->tail = l->head;
    assign_value(L, 2, &l->head->value, &type);
  } else {
    l_node *head = malloc(sizeof(l_node));
    head->next = l->head;
    l->head->prev = head;
    assign_value(L, 2, &head->value, &type);
    l->head = head;
    head = NULL;
  }
  l->head->prev = NULL;
  l->head->type = type;
  l->size++;
  return 1;
}

/* pop an node */
static int list_pop(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- cannot pop any node!", l->size);
  }
  lua_pushlistval(L, l->tail);
  remove_tail(l);
  return 1;
}

/* pop an node at head */
static int list_popleft(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- cannot pop any node!", l->size);
  }
  lua_pushlistval(L, l->head);
  remove_head(l);
  return 1;
}

/* get node of list by index */
static int list_get(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  // check size
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- no node in the list!", l->size);
  }
  // check index
  lua_Integer idx = luaL_checkinteger(L, 2);
  if (idx <= 0 || idx > l->size) {
    luaL_error(L, "list size: %d --- index %d out of range!", l->size, idx);
  }
  // get the correct nodesor
  lua_pushlistval(L, get_node(l, idx));
  return 1;
}

/* remove an node */
static int list_remove(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  // check size
  if (l->size <= 0) {
    luaL_error(L, "list size: %d --- no node in the list!", l->size);
  }
  // check index
  lua_Integer idx = luaL_checkinteger(L, 2);
  if (idx <= 0 || idx > l->size) {
    luaL_error(L, "list size: %d --- index %d out of range!", l->size, idx);
  }
  remove_node(l, idx);
  return 1;
}

/* get size of the list */
static int list_size(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
  lua_Integer size = (lua_Integer)(l->size);
  lua_settop(L, 0);
  lua_pushinteger(L, size);
  return 1;
}

/* reverse list */
static int list_reverse(lua_State *L) {
  l_list *l = lua_checklist(L, 1);
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

// static int list_pairs(lua_State *L) {

// }


static const luaL_Reg list_lib[] = {
    {"new", list_new},
    {"push", list_push},
    {"pushleft", list_pushleft},
    {"pop", list_pop},
    {"popleft", list_popleft},
    {"remove", list_remove},
    {"get", list_get},
    {"size", list_size},
    {"reverse", list_reverse},
    // {"__pairs", list_pairs},
    {NULL, NULL},
};

LUAMOD_API int luaopen_list(lua_State *L) {
  lua_newtable(L);
  luaL_setfuncs(L, list_lib, 0);
  return 1;
}
