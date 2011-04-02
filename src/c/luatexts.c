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

/* Really spammy SPAM */
#if 0
  #define XSPAM(a) printf a
#else
  #define XSPAM(a) (void)0
#endif

/* Regular SPAM */
#if 1
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

/* Error-message SPAM */
#if 0
  #define ESPAM(a) printf a
#else
  #define ESPAM(a) (void)0
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

/*
* UTF-8 handling implemented based on information here:
* http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
*/

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
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
   4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  0,  0
};

/*
* *Increments* len_bytes by the number of bytes read.
* Fails on invalid UTF-8 characters.
*/
static int ltsLS_eatutf8char(lts_LoadState * ls, size_t * len_bytes)
{
  unsigned char b = 0;
  signed char expected_length = 0;
  int i = 0;
  const unsigned char * origin = ls->pos;

  /* Check if we have any data in the buffer */
  if (!ltsLS_good(ls) || ltsLS_unread(ls) < 1)
  {
    ESPAM(("eatutf8char: no buffer to start with\n"));
    ls->unread = 0;
    ls->pos = NULL;
    return LUATEXTS_ECLIPPED;
  }

  /* We have at least one byte in the buffer, let's check it out. */
  b = *ls->pos;

  /* We did just eat a byte, no matter what happens next. */
  ++ls->pos;
  --ls->unread;

  /* Get an expected length of a character. */
  expected_length = utf8_char_len[b];
  XSPAM((
      "eatutf8char: first byte 0x%X (%d) expected length: %d\n",
      b, b, expected_length
    ));

  /* Check if it was a valid first byte. */
  if (expected_length < 1)
  {
    ESPAM(("eatutf8char: invalid start byte 0x%X (%d)\n", b, b));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }

  /* If it was a single-byte ASCII character, return right away. */
  if (expected_length == 1)
  {
    XSPAM(("eatutf8char: ascii 0x%X (%d)\n", b, b));

    *len_bytes += expected_length;

    return LUATEXTS_ESUCCESS;
  }

  /*
  * It was a multi-byte character. Check if we have enough bytes unread.
  * Note that we've eaten one byte already.
  */
  if (ltsLS_unread(ls) + 1 < (unsigned char)expected_length)
  {
    ESPAM(("eatutf8char: multibyte character clipped\n"));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_ECLIPPED; /* Assuming it is buffer's fault. */
  }

  /* Let's eat the rest of characters */
  for (i = 1; i < expected_length; ++i)
  {
    b = *ls->pos;

    /* We did just eat a byte, no matter what happens next. */
    ++ls->pos;
    --ls->unread;

    XSPAM(("eatutf8char: cont 0x%X (%d)\n", b, b));

    /* Check if it is a continuation byte */
    if (utf8_char_len[b] != -1)
    {
      ESPAM(("eatutf8char: invalid continuation byte 0x%X (%d)\n", b, b));

      ls->unread = 0;
      ls->pos = NULL;

      return LUATEXTS_EBADUTF8;
    }
  }

  /* All bytes are correct, let's check out for overlong forms */
  if (
      expected_length == 2 && (
          (origin[0] & 0xFE) == 0xC0
        )
    )
  {
    ESPAM((
        "eatutf8char: overlong form 2 0x%X 0x%X (%d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }
  else if (
      expected_length == 3 && (
          origin[0] == 0xE0
            && (origin[1] & 0xE0) == 0x80
        )
    )
  {
    ESPAM((
        "eatutf8char: overlong form 3 0x%X 0x%X 0x%X (%d %d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1],
        origin[2], origin[2]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }
  else if (
      expected_length == 4 && (
          origin[0] == 0xF0
            && (origin[1] & 0xF0) == 0x80
        )
    )
  {
    ESPAM((
        "eatutf8char: overlong form 4 0x%X 0x%X 0x%X 0x%X (%d %d %d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1],
        origin[2], origin[2],
        origin[3], origin[3]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }
  else if (
      expected_length == 5 && (
          origin[0] == 0xF8
            && (origin[1] & 0xF8) == 0x80
        )
    )
  {
    ESPAM((
        "eatutf8char: overlong form 5"
        " 0x%X 0x%X 0x%X 0x%X 0x%X (%d %d %d %d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1],
        origin[2], origin[2],
        origin[3], origin[3],
        origin[4], origin[4]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }
  else if (
      expected_length == 6 && (
          origin[0] == 0xFC
            && (origin[1] & 0xFC) == 0x80
        )
    )
  {
    ESPAM((
        "eatutf8char: overlong form 6"
        " 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X (%d %d %d %d %d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1],
        origin[2], origin[2],
        origin[3], origin[3],
        origin[4], origin[4],
        origin[5], origin[5]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }

  /* No overlongs, check for surrogates. */

  if (
      expected_length == 3 && (
          origin[0] == 0xED
            && (origin[1] & 0xE0) == 0xA0
        )
    )
  {
    ESPAM((
        "eatutf8char: surrogate form 3 0x%X 0x%X 0x%X (%d %d %d)\n",
        origin[0], origin[0],
        origin[1], origin[1],
        origin[2], origin[2]
      ));

    ls->unread = 0;
    ls->pos = NULL;

    return LUATEXTS_EBADUTF8;
  }

  /*
  * Note: Not checking for U+FFFE or U+FFFF.
  *
  * Chapter 3 of version 3.2 of the Unicode standard, Paragraph C5 says
  * "A process shall not interpret either U+FFFE or U+FFFF as an abstract
  * character", but table 3.1B includes them among
  * the "Legal UTF-8 Byte Sequences".
  *
  * We opt to pass them through.
  */

  /* Phew. All done, the UTF-8 character is valid. */

  XSPAM(("eatutf8char: read one char successfully\n"));

  *len_bytes += expected_length;

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
