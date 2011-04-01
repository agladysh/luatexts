/*
* luatexts.c: Trivial Lua human-readable binary-safe serialization library
*             See copyright information in file COPYRIGHT.
*/

#if defined (__cplusplus)
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>

#include <stdlib.h>

#include "luainternals.h"

#if defined (__cplusplus)
}
#endif

#if 0
  #define XSPAM(a) printf a
#else
  #define XSPAM(a) (void)0
#endif

#if 0
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

#define LUATEXTS_VERSION     "luatexts 0.1"
#define LUATEXTS_COPYRIGHT   "Copyright (C) 2011, luatexts authors"
#define LUATEXTS_DESCRIPTION \
    "Trivial Lua human-readable binary-safe serialization library"

#define LUATEXTS_ESUCCESS (0)
#define LUATEXTS_EFAILURE (1)
#define LUATEXTS_EBADSIZE (2)
#define LUATEXTS_EBADDATA (3)
#define LUATEXTS_EBADTYPE (4)
#define LUATEXTS_EGARBAGE (5)
#define LUATEXTS_ETOOHUGE (6)
#define LUATEXTS_EBADUTF8 (7)
#define LUATEXTS_ECLIPPED (8)

#define LUATEXTS_CNIL        '-'
#define LUATEXTS_CFALSE      '0'
#define LUATEXTS_CTRUE       '1'
#define LUATEXTS_CNUMBER     'N'
#define LUATEXTS_CUINT       'U'
#define LUATEXTS_CSTRING     'S'
#define LUATEXTS_CTABLE      'T'
#define LUATEXTS_CSTRINGUTF8 '8'

/* WARNING: Make sure these match your luaconf.h */
typedef lua_Number LUATEXTS_NUMBER;
typedef unsigned long LUATEXTS_UINT;

#define luatexts_tonumber strtod
#define luatexts_touint strtoul

typedef struct lts_LoadState
{
  const unsigned char * pos;
  size_t unread;
} lts_LoadState;

static void ltsLS_init(
    lts_LoadState * ls,
    const unsigned char * data,
    size_t len
  )
{
  ls->pos = (len > 0) ? data : NULL;
  ls->unread = len;
}

#define ltsLS_good(ls) \
  ((ls)->pos != NULL)

#define ltsLS_unread(ls) \
  ((ls)->unread)

static const unsigned char * ltsLS_eat(lts_LoadState * ls, size_t len)
{
  const unsigned char * result = NULL;
  if (ltsLS_good(ls))
  {
    if (ltsLS_unread(ls) >= len)
    {
      result = ls->pos;
      ls->pos += len;
      ls->unread -= len;
      XSPAM(("XXX %lu '%s'\n", (unsigned long) len, result));
    }
    else
    {
      ls->unread = 0;
      ls->pos = NULL;
    }
  }

  return result;
}

/* Based on this: http://en.wikipedia.org/wiki/UTF-8#Codepage_layout */
static const signed char utf8_char_len[256] =
{
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   0,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
   4,  4,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const unsigned char utf8_min_values[] =
{
  0, 0xC1, 0xE1, 0xF1
};

/*
* *Increments* len_bytes by the number of bytes read.
* Fails on invalid UTF-8 characters.
*/
static int ltsLS_eatutf8char(lts_LoadState * ls, size_t * len_bytes)
{
  signed char expected_len = 0;
  int i = 0;
  unsigned char b = 0;
  const unsigned char * origin = ls->pos;
  int had_data = 0;

  SPAM(("UTF8 BEGIN CHAR\n"));

  if (!ltsLS_good(ls) || ltsLS_unread(ls) < 1)
  {
    ls->unread = 0;
    ls->pos = NULL;
    SPAM(("UTF8 CLIPPED\n"));
    return LUATEXTS_ECLIPPED; /* Run out of buffer */
  }

  /* First byte must be a start one */
  b = *ls->pos;

  /* We've read a single byte now, no matter what */
  ++ls->pos;
  --ls->unread;

  /* Look up number of bytes in character */
  expected_len = utf8_char_len[b];
  if (expected_len < 1)
  {
    /* Bad start byte, or unexpected continuation byte */
    ls->unread = 0;
    ls->pos = NULL;
    SPAM(("UTF8 bsboucb 0x%X (%d)\n", b, b));
    return LUATEXTS_EBADUTF8;
  }

  /* So, we've got a correct start byte. */
  SPAM(("UTF8 BYTE 0x%X (%d) START LEN %d\n", b, b, expected_len));

  /* Maybe it is a single-byte character? */
  if (expected_len == 1)
  {
    SPAM(("UTF8 END CHAR ASCII 0x%X (%d)\n", b, b));

    *len_bytes += expected_len;
    return LUATEXTS_ESUCCESS;
  }

  /*
  * It is a multi-byte character, let's check if we have all bytes we need.
  * Note that we did read one byte above.
  */
  if (ltsLS_unread(ls) + 1 < expected_len)
  {
    /*
    * Not enough bytes in buffer to read whole character.
    */
    ls->unread = 0;
    ls->pos = NULL;
    SPAM(("UTF8 clipped\n"));
    return LUATEXTS_ECLIPPED;
  }

  had_data = (b > utf8_min_values[expected_len - 1]);
  SPAM(("UTF8 INITAL HD %d > %d = %d\n", b, utf8_min_values[expected_len - 1], had_data));

  /*
  * We do have enough bytes in buffer.
  * Let's look ahead and check that all of them are continuation ones.
  */
  for (i = 1; i < expected_len; ++i)
  {
    b = *ls->pos;

    /* We've read a single byte now, no matter what. */
    ++ls->pos;
    --ls->unread;

    if (b != 0x80)
    {
      had_data = 1;
    }
    else if (!had_data)
    {
      if (expected_len != 2)
      {
        /* Not a shortest form. Broken UTF-8. */
        ls->unread = 0;
        ls->pos = NULL;
        SPAM(("UTF8 not a shortest form 0x%X (%d)\n", b, b));
        return LUATEXTS_EBADUTF8;
      }
    }

    if (utf8_char_len[b] != -1)
    {
      /* Not a continuation byte. Broken UTF-8. */
      ls->unread = 0;
      ls->pos = NULL;
      SPAM(("UTF8 nacbbu8 0x%X (%d)\n", b, b));
      return LUATEXTS_EBADUTF8;
    }

    SPAM(("UTF8 BYTE 0x%X (%d) CONT\n", b, b));
  }

  /* Phew. Eaten whole character successfully. */

  SPAM(("UTF8 END CHAR %d\n", expected_len));

  *len_bytes += expected_len;
  return LUATEXTS_ESUCCESS;
}

/*
* Eats specified number of UTF-8 characters. Returns NULL if failed.
* Fails on invalid UTF-8 characters.
*/
static int ltsLS_eatutf8(
    lts_LoadState * ls,
    size_t num_chars,
    const unsigned char ** dest,
    size_t * len_bytes
  )
{
  const unsigned char * origin = ls->pos;
  size_t num_bytes = 0;
  size_t i = 0;
  int result = LUATEXTS_ESUCCESS;

  for (i = 0; i < num_chars; ++i)
  {
    result = ltsLS_eatutf8char(ls, &num_bytes);
    if (result != LUATEXTS_ESUCCESS)
    {
      return result;
    }
  }

  *dest = origin;
  *len_bytes = num_bytes;

  return result;
}

/*
* Returned length does not include trailing '\r\n' (or '\n' if '\r' is missing).
* It is guaranteed that on success it is safe to read byte at (*dest + len + 1)
* and that byte is either '\r' or '\n'.
*/
static int ltsLS_readline(
    lts_LoadState * ls,
    const unsigned char ** dest,
    size_t * len
  )
{
  const unsigned char * origin = ls->pos;
  unsigned char last = 0;
  size_t read = 0;

  while (ltsLS_good(ls))
  {
    if (ltsLS_unread(ls) > 0)
    {
      unsigned char b = *ls->pos;
      ++ls->pos;
      --ls->unread;

      XSPAM(("LINE CHAR 0x%X\n", b));

      if (b == '\n')
      {
        *dest = origin;
        *len = (last == '\r') ? read - 1 : read;

        XSPAM((
            "LINE END %s read %lu\n",
            (last == '\r') ? "CRLF" : "LF",
            (long unsigned int)read
          ));

        return LUATEXTS_ESUCCESS;
      }

      last = b;
      ++read;
    }
    else
    {
      ls->unread = 0;
      ls->pos = NULL;
    }
  }

  return LUATEXTS_ECLIPPED;
}

static int ltsLS_readuint(lts_LoadState * ls, LUATEXTS_UINT * dest)
{
  size_t len = 0;
  const unsigned char * data = NULL;
  int result = ltsLS_readline(ls, &data, &len);

  XSPAM(("UINT readline %d\n", result));

  if (result == LUATEXTS_ESUCCESS && len == 0)
  {
    result = LUATEXTS_EBADDATA;
  }

  if (result == LUATEXTS_ESUCCESS)
  {
    /*
    * This is safe, since we're guaranteed to have
    * at least one non-numeric trailing byte.
    */
    char * endptr = NULL;
    LUATEXTS_UINT value = luatexts_touint((const char *)data, &endptr, 10);
    if ((const unsigned char *)endptr != data + len)
    {
      XSPAM((
          "UINT GARBAGE end %p start %p len %lu\n",
          endptr, data, (long unsigned int)len
        ));
      result = LUATEXTS_EGARBAGE;
    }
    else
    {
      if ((lua_Integer)value < 0) /* Implementation detail */
      {
        result = LUATEXTS_ETOOHUGE;
      }
      else
      {
        *dest = value;
      }
    }
  }

  return result;
}

static int ltsLS_readnumber(lts_LoadState * ls, LUATEXTS_NUMBER * dest)
{
  size_t len = 0;
  const unsigned char * data = NULL;
  int result = ltsLS_readline(ls, &data, &len);

  if (result == LUATEXTS_ESUCCESS && len == 0)
  {
    result = LUATEXTS_EBADDATA;
  }

  if (result == LUATEXTS_ESUCCESS)
  {
    /*
    * This is safe, since we're guaranteed to have
    * at least one non-numeric trailing byte.
    */
    char * endptr = NULL;
    LUATEXTS_NUMBER value = strtod((const char *)data, &endptr);
    XSPAM(("XXX NUM %.54g\n", value));
    if ((const unsigned char *)endptr != data + len)
    {
      result = LUATEXTS_EGARBAGE;
    }
    else
    {
      *dest = value;
    }
  }

  return result;
}

static int load_value(lua_State * L, lts_LoadState * ls);

static int load_table(lua_State * L, lts_LoadState * ls)
{
  LUATEXTS_UINT array_size = 0;
  LUATEXTS_UINT hash_size = 0;
  LUATEXTS_UINT total_size = 0;

  int result = ltsLS_readuint(ls, &array_size);

  if (result == LUATEXTS_ESUCCESS)
  {
    result = ltsLS_readuint(ls, &hash_size);
  }

  if (result == LUATEXTS_ESUCCESS)
  {
    total_size = array_size + hash_size;

    /*
    * Security note: not checking for unread buffer size underflow.
    * Motivation: will fail at read time, so why bother?
    */

    if (
        array_size < 0 || array_size > MAXASIZE ||
        hash_size < 0  ||
        (hash_size > 0 && ceillog2((unsigned int)hash_size) > MAXBITS)
      )
    {
      result = LUATEXTS_ETOOHUGE;
    }
  }

  if (result == LUATEXTS_ESUCCESS)
  {
    LUATEXTS_UINT i = 0;

    lua_createtable(L, array_size, hash_size);

    for (i = 0; i < total_size; ++i)
    {
      int key_type = LUA_TNONE;

      result = load_value(L, ls); /* Load key. */
      if (result != LUATEXTS_ESUCCESS)
      {
        break;
      }

      /* Table key can't be nil or NaN */
      key_type = lua_type(L, -1);
      if (key_type == LUA_TNIL)
      {
        /* Corrupt data? */
        result = LUATEXTS_EBADDATA;
        break;
      }

      if (key_type == LUA_TNUMBER)
      {
        lua_Number key = lua_tonumber(L, -1);
        if (luai_numisnan(key))
        {
          /* Corrupt data? */
          result = LUATEXTS_EBADDATA;
          break;
        }
      }

      result = load_value(L, ls); /* Load value. */
      if (result != LUATEXTS_ESUCCESS)
      {
        break;
      }

      lua_rawset(L, -3);
    }
  }

  return result;
}

static int load_value(lua_State * L, lts_LoadState * ls)
{
  size_t len = 0;
  const unsigned char * type = NULL;

  /* Read value type */
  int result = ltsLS_readline(ls, &type, &len);
  if (result == LUATEXTS_ESUCCESS && len != 1)
  {
    result = LUATEXTS_EGARBAGE;
  }

  /* Read value data */
  if (result == LUATEXTS_ESUCCESS)
  {
    XSPAM(("VALUE TYPE '%c'\n", type[0]));
    switch (type[0])
    {
      case LUATEXTS_CNIL:
        lua_pushnil(L);
        break;

      case LUATEXTS_CFALSE:
        lua_pushboolean(L, 0);
        break;

      case LUATEXTS_CTRUE:
        lua_pushboolean(L, 1);
        break;

      case LUATEXTS_CNUMBER:
        {
          lua_Number value;

          result = ltsLS_readnumber(ls, &value);
          if (result == LUATEXTS_ESUCCESS)
          {
            lua_pushnumber(L, value);
          }
        }
        break;

      case LUATEXTS_CUINT:
        {
          LUATEXTS_UINT value;

          result = ltsLS_readuint(ls, &value);
          if (result == LUATEXTS_ESUCCESS)
          {
            XSPAM(("VALUE UINT %lu\n", value));
            lua_pushinteger(L, value);
          }
        }
        break;

      case LUATEXTS_CSTRING:
        {
          const unsigned char * str = NULL;

          /* Read string size */
          LUATEXTS_UINT len = 0;
          result = ltsLS_readuint(ls, &len);

          /* Read string data */
          if (result == LUATEXTS_ESUCCESS)
          {
            str = ltsLS_eat(ls, len);
            if (str == NULL)
            {
              result = LUATEXTS_EBADSIZE;
            }
          }

          /* Eat newline after string data */
          if (result == LUATEXTS_ESUCCESS)
          {
            size_t empty = 0;
            const unsigned char * nl = NULL;
            result = ltsLS_readline(ls, &nl, &empty);
            if (result == LUATEXTS_ESUCCESS && empty != 0)
            {
              result = LUATEXTS_EGARBAGE;
            }
          }

          if (result == LUATEXTS_ESUCCESS)
          {
            lua_pushlstring(L, (const char *)str, len);
          }
        }
        break;

      case LUATEXTS_CSTRINGUTF8:
        {
          const unsigned char * str = NULL;

          /* Read string size */
          LUATEXTS_UINT len_chars = 0;
          size_t len_bytes = 0;
          result = ltsLS_readuint(ls, &len_chars);

          /* Read string data */
          if (result == LUATEXTS_ESUCCESS)
          {
            result = ltsLS_eatutf8(ls, len_chars, &str, &len_bytes);
          }

          /* Eat newline after string data */
          if (result == LUATEXTS_ESUCCESS)
          {
            size_t empty = 0;
            const unsigned char * nl = NULL;
            result = ltsLS_readline(ls, &nl, &empty);
            if (result == LUATEXTS_ESUCCESS && empty != 0)
            {
              result = LUATEXTS_EGARBAGE;
            }
          }

          if (result == LUATEXTS_ESUCCESS)
          {
            lua_pushlstring(L, (const char *)str, len_bytes);
          }
        }
        break;

      case LUATEXTS_CTABLE:
        result = load_table(L, ls);
        break;

      default:
        result = LUATEXTS_EBADTYPE;
        break;
    }
  }

  XSPAM(("LOAD VALUE result %d\n", result));

  return result;
}

static int luatexts_load(
    lua_State * L,
    const unsigned char * buf,
    size_t len,
    size_t * count
  )
{
  int result = LUATEXTS_ESUCCESS;
  lts_LoadState ls;

  LUATEXTS_UINT tuple_size = 0;
  LUATEXTS_UINT i = 0;

  int base = lua_gettop(L);

  ltsLS_init(&ls, buf, len);

  /*
  * Security note: not checking tuple_size for sanity.
  * Motivation: too complicated; we will fail if buffer is too small anyway.
  */

  result = ltsLS_readuint(&ls, &tuple_size);
  for (i = 0; i < tuple_size && result == LUATEXTS_ESUCCESS; ++i)
  {
    result = load_value(L, &ls);
  }

  /*
  * Security note: ignoring unread bytes if any.
  * Motivation: more casual, this is a text format after all.
  */

  if (result == LUATEXTS_ESUCCESS)
  {
    *count = tuple_size;
  }
  else
  {
    XSPAM(("LOAD TUPLE error %d\n", result));
    lua_settop(L, base); /* Discard intermediate results */

    switch (result)
    {
      case LUATEXTS_EBADSIZE:
        lua_pushliteral(L, "load failed: corrupt data, bad size");
        break;

      case LUATEXTS_EBADDATA:
        lua_pushliteral(L, "load failed: corrupt data");
        break;

      case LUATEXTS_EBADTYPE:
        lua_pushliteral(L, "load failed: unknown data type");
        break;

      case LUATEXTS_EGARBAGE:
        lua_pushliteral(L, "load failed: garbage before newline");
        break;

      case LUATEXTS_ETOOHUGE:
        lua_pushliteral(L, "load failed: value too huge");
        break;

      case LUATEXTS_EBADUTF8:
        lua_pushliteral(L, "load failed: invalid utf-8 data");
        break;

      case LUATEXTS_ECLIPPED:
        lua_pushliteral(L, "load failed: corrupt data, truncated");
        break;

      /* should not happen */
      case LUATEXTS_EFAILURE:
      default:
        lua_pushliteral(L, "load failed: internal error");
        break;
    }
  }

  return result;
}

static int lload(lua_State * L)
{
  size_t len = 0;
  const unsigned char * buf = (const unsigned char *)luaL_checklstring(
      L, 1, &len
    );
  size_t tuple_size = 0;
  int result = 0;

  lua_pushboolean(L, 1);

  result = luatexts_load(L, buf, len, &tuple_size);
  if (result != LUATEXTS_ESUCCESS)
  {
    lua_pushnil(L);
    lua_replace(L, -3); /* Replace pre-pushed true with nil */
    return 2; /* Error message already on stack */
  }

  return tuple_size + 1;
}

/* Lua module API */
static const struct luaL_reg R[] =
{
  { "load", lload },

  { NULL, NULL }
};

#ifdef __cplusplus
extern "C" {
#endif

LUALIB_API int luaopen_luatexts(lua_State * L)
{
  /*
  * Register module
  */
  luaL_register(L, "luatexts", R);

  /*
  * Register module information
  */
  lua_pushliteral(L, LUATEXTS_VERSION);
  lua_setfield(L, -2, "_VERSION");

  lua_pushliteral(L, LUATEXTS_COPYRIGHT);
  lua_setfield(L, -2, "_COPYRIGHT");

  lua_pushliteral(L, LUATEXTS_DESCRIPTION);
  lua_setfield(L, -2, "_DESCRIPTION");

  return 1;
}

#ifdef __cplusplus
}
#endif
