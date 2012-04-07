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

/* TODO: Hide this mmap stuff in a separate file */
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#if defined (__cplusplus)
}
#endif

#define DO_XSPAM  0
#define DO_XESPAM 0
#define DO_SPAM   0
#define DO_ESPAM  0

/* Really spammy SPAM */
#if DO_XSPAM
  #include <ctype.h>
  #define XSPAM(a) printf a
#else
  #define XSPAM(a) (void)0
#endif

/* Really spammy error-message SPAM */
#if DO_XESPAM
  #include <ctype.h>
  #define XESPAM(a) printf a
#else
  #define XESPAM(a) (void)0
#endif

/* Regular SPAM */
#if DO_SPAM
  #include <ctype.h>
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

/* Error-message SPAM */
#if DO_ESPAM
  #include <ctype.h>
  #define ESPAM(a) printf a
#else
  #define ESPAM(a) (void)0
#endif

#define LUATEXTS_VERSION     "luatexts 0.1.4"
#define LUATEXTS_COPYRIGHT   "Copyright (C) 2011-2012, luatexts authors"
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

#define LUATEXTS_CNIL         '-' /* 0x2D (45)  */
#define LUATEXTS_CFALSE       '0' /* 0x30 (48)  */
#define LUATEXTS_CTRUE        '1' /* 0x31 (49)  */
#define LUATEXTS_CNUMBER      'N' /* 0x4E (78)  */
#define LUATEXTS_CUINT        'U' /* 0x55 (85)  */
#define LUATEXTS_CUINTHEX     'H' /* 0x48 (48)  */
#define LUATEXTS_CUINT36      'Z' /* 0x5A (90)  */
#define LUATEXTS_CSTRING      'S' /* 0x53 (83)  */
#define LUATEXTS_CFIXEDTABLE  'T' /* 0x54 (84)  */
#define LUATEXTS_CSTREAMTABLE 't' /* 0x74 (116) */
#define LUATEXTS_CSTRINGUTF8  '8' /* 0x38 (56)  */

/* WARNING: Make sure these match your luaconf.h */
typedef lua_Number LUATEXTS_NUMBER;
typedef unsigned long LUATEXTS_UINT;

#define luatexts_tonumber strtod

#if defined(__GNUC__)

#define LUATEXTS_LIKELY(x)    __builtin_expect(!!(x), 1)
#define LUATEXTS_UNLIKELY(x)  __builtin_expect(!!(x), 0)

#endif /* defined(__GNUC__) */

#ifndef LUATEXTS_LIKELY

#define LUATEXTS_LIKELY(x)    (x)
#define LUATEXTS_UNLIKELY(x)  (x)

#endif /* !defined(LUATEXTS_LIKELY) */

#define LUATEXTS_CONCAT_(lhs, rhs) lhs ## rhs
#define LUATEXTS_CONCAT(lhs, rhs) LUATEXTS_CONCAT_(lhs, rhs)

#define LUATEXTS_STRINGIFY_(a) #a
#define LUATEXTS_STRINGIFY(a) LUATEXTS_STRINGIFY_(a)

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

#define ltsLS_close(ls) \
  do { \
    (ls)->unread = 0; \
    (ls)->pos = NULL; \
  } while (0)

static const unsigned char * ltsLS_eat(lts_LoadState * ls, size_t len)
{
  const unsigned char * result = NULL;
  if (LUATEXTS_LIKELY(ltsLS_good(ls)))
  {
    if (LUATEXTS_LIKELY(ltsLS_unread(ls) >= len))
    {
      result = ls->pos;
      ls->pos += len;
      ls->unread -= len;
    }
    else
    {
      ltsLS_close(ls);
      return NULL;
    }
  }

  return result;
}

#define LUATEXTS_FAIL(ls, status, msg) \
  do { \
    ESPAM(msg); \
    ltsLS_close(ls); \
    return (status); \
  } while (0)

#define LUATEXTS_ENSURE(ls, x, status, msg) \
  do { \
    if (LUATEXTS_UNLIKELY(!(x))) \
    { \
      LUATEXTS_FAIL(ls, status, msg); \
    } \
  } while (0)

#define EAT_CHAR(ls, msg) \
  do { \
    LUATEXTS_ENSURE(ls, \
        ltsLS_unread((ls)) > 0, \
        LUATEXTS_ECLIPPED, (msg ": clipped\n") \
      ); \
    ++(ls)->pos; \
    --(ls)->unread; \
  } while (0)

#define EAT_NEWLINE(ls, msg) \
  do { \
    if (*(ls)->pos == '\r') \
    { \
      EAT_CHAR((ls), msg); \
    } \
    LUATEXTS_ENSURE((ls), \
        *(ls)->pos == '\n', \
        LUATEXTS_EGARBAGE, (msg ": garbage\n") \
      ); \
    EAT_CHAR((ls), msg); \
  } while (0);

/*
* UTF-8 handling implemented based on information here:
* http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
*
* Updated with the information here:
*
* http://codereview.stackexchange.com/q/1624/234
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
   0,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
   4,  4,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/*
* (From Chapter 3 of the 6.0.0 Unicode Standard)
*
* Table 3-7. Well-Formed UTF-8 Byte Sequences
* Code Points         First Byte  Second Byte  Third Byte  Fourth Byte
* U+0000..U+007F      00..7F
* U+0080..U+07FF      C2..DF      80..BF
* U+0800..U+0FFF      E0          A0..BF       80..BF
* U+1000..U+CFFF      E1..EC      80..BF       80..BF
* U+D000..U+D7FF      ED          80..9F       80..BF
* U+E000..U+FFFF      EE..EF      80..BF       80..BF
* U+10000..U+3FFFF    F0          90..BF       80..BF       80..BF
* U+40000..U+FFFFF    F1..F3      80..BF       80..BF       80..BF
* U+100000..U+10FFFF  F4          80..8F       80..BF       80..BF
*/

/*
* *Increments* len_bytes by the number of bytes read.
* Fails on invalid UTF-8 characters.
*/
static int ltsLS_eatutf8char(lts_LoadState * ls, size_t * len_bytes)
{
  unsigned char b = 0;
  signed char exp_len = 0;
  int i = 0;
  const unsigned char * origin = ls->pos;

  /* Check if we have any data in the buffer */
  LUATEXTS_ENSURE(ls,
      ltsLS_good(ls) && ltsLS_unread(ls) >= 1,
      LUATEXTS_ECLIPPED, ("eatutf8char: no buffer to start with\n")
    );

  /* We have at least one byte in the buffer, let's check it out. */
  b = *ls->pos;

  /* We did just eat a byte, no matter what happens next. */
  ++ls->pos;
  --ls->unread;

  /* Get an expected length of a character. */
  exp_len = utf8_char_len[b];
  XSPAM((
      "eatutf8char: first byte 0x%X (%d) expected length: %d\n",
      b, b, exp_len
    ));

  /* Check if it was a valid first byte. */
  LUATEXTS_ENSURE(ls,
      exp_len >= 1,
      LUATEXTS_EBADUTF8, ("eatutf8char: invalid start byte 0x%X (%d)\n", b, b)
    );

  /* If it was a single-byte ASCII character, return right away. */
  if (LUATEXTS_LIKELY(exp_len == 1))
  {
    XSPAM(("eatutf8char: ascii 0x%X (%d)\n", b, b));

    *len_bytes += exp_len;

    return LUATEXTS_ESUCCESS;
  }

  /*
  * It was a multi-byte character. Check if we have enough bytes unread.
  * Note that we've eaten one byte already.
  */
  LUATEXTS_ENSURE(ls,
      ltsLS_unread(ls) + 1 >= (unsigned char)exp_len,
      LUATEXTS_ECLIPPED, ("eatutf8char: multibyte character clipped")
    );

  /* Let's eat the rest of characters */
  for (i = 1; i < exp_len; ++i)
  {
    b = *ls->pos;

    /* We did just eat a byte, no matter what happens next. */
    ++ls->pos;
    --ls->unread;

    XSPAM(("eatutf8char: cont 0x%X (%d)\n", b, b));

    /* Check if it is a continuation byte */
    LUATEXTS_ENSURE(ls,
        utf8_char_len[b] == -1,
        LUATEXTS_EBADUTF8,
        ("eatutf8char: invalid continuation byte 0x%X (%d)\n", b, b)
      );
  }

  /* All bytes are correct; check out for overlong forms and surrogates */
  LUATEXTS_ENSURE(ls,
      !(
        (exp_len == 2 && ((origin[0]  & 0xFE) == 0xC0))                      ||
        (exp_len == 3 &&  (origin[0] == 0xE0 && (origin[1] & 0xE0) == 0x80)) ||
        (exp_len == 4 &&  (origin[0] == 0xF0 && (origin[1] & 0xF0) == 0x80)) ||
        (exp_len == 4 &&  (origin[0] == 0xF4 && (origin[1] > 0x8F)))         ||
        (exp_len == 3 &&  (origin[0] == 0xED && (origin[1] & 0xE0) != 0x80))
      ),
      LUATEXTS_EBADUTF8, ("eatutf8char: overlong or surrogate detected\n")
    );

  /* Reject BOM non-characters: U+FFFE and U+FFFF */
  LUATEXTS_ENSURE(ls,
      !(
        exp_len == 3 && (
            (origin[0] == 0xEF && origin[1] == 0xBF && origin[2] == 0xBE) ||
            (origin[0] == 0xEF && origin[1] == 0xBF && origin[2] == 0xBF)
          )
      ),
      LUATEXTS_EBADUTF8, ("eatutf8char: BOM detected\n")
    );

  /* Phew. All done, the UTF-8 character is valid. */

  XSPAM(("eatutf8char: read one char successfully\n"));

  *len_bytes += exp_len;

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

    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("eatutf8: failed to eat char %lu\n", (long unsigned int)i));
      return result;
    }
  }

  *dest = origin;
  *len_bytes = num_bytes;

  return LUATEXTS_ESUCCESS;
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
  const unsigned char * last = NULL;
  size_t read = 0;

  while (ltsLS_good(ls))
  {
    if (LUATEXTS_LIKELY(ltsLS_unread(ls) > 0))
    {
      const unsigned char * cur = ls->pos;
      ++ls->pos;
      --ls->unread;

      if (LUATEXTS_UNLIKELY(*cur == '\n'))
      {
        *dest = origin;
        *len = (last != NULL && *last == '\r') ? read - 1 : read;

        return LUATEXTS_ESUCCESS;
      }

      last = cur;
      ++read;
    }
    else
    {
      ltsLS_close(ls);
      break;
    }
  }

  ESPAM(("readline: clipped\n"));

  return LUATEXTS_ECLIPPED;
}

static const signed char uint_lookup_table_10[256] =
{
/*  0*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 16*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 32*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 48*/     0,  1,  2,  3,  4,  5,  6,  7,  8 , 9 ,-1, -1, -1, -1, -1, -1,
/* 64*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 80*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 96*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*112*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*128*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*144*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*160*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*176*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*192*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*208*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*224*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*240*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1
};

static const signed char uint_lookup_table_16[256] =
{
/*  0*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 16*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 32*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 48*/     0,  1,  2,  3,  4,  5,  6,  7,  8 , 9 ,-1, -1, -1, -1, -1, -1,
/* 64*/    -1, 10, 11, 12, 13, 14, 15, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 80*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 96*/    -1, 10, 11, 12, 13, 14, 15, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*112*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*128*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*144*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*160*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*176*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*192*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*208*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*224*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*240*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1
};

static const signed char uint_lookup_table_36[256] =
{
/*  0*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 16*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 32*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/* 48*/     0,  1,  2,  3,  4,  5,  6,  7,  8 , 9 ,-1, -1, -1, -1, -1, -1,
/* 64*/    -1, 10, 11, 12, 13, 14, 15, 16, 17 ,18 ,19, 20, 21, 22, 23, 24,
/* 80*/    25, 26, 27, 28, 29, 30, 31, 32, 33 ,34 ,35, -1, -1, -1, -1, -1,
/* 96*/    -1, 10, 11, 12, 13, 14, 15, 16, 17 ,18 ,19, 20, 21, 22, 23, 24,
/*112*/    25, 26, 27, 28, 29, 30, 31, 32, 33 ,34 ,35, -1, -1, -1, -1, -1,
/*128*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*144*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*160*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*176*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*192*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*208*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*224*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1,
/*240*/    -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1, -1, -1, -1, -1, -1
};

/*
  Not supporting leading '-' (this is unsigned int)
  and not eating leading whitespace
  (it is accidental that strtoul eats it,
  luatexts format formally does not support this).
*/
#define DECLARE_READUINT(ltsLS_readuint, BASE, LIMIT, TAIL) \
  static int LUATEXTS_CONCAT(ltsLS_readuint, BASE)( \
      lts_LoadState * ls, \
      LUATEXTS_UINT * dest \
    ) \
  { \
    LUATEXTS_UINT k = 0; \
    if (LUATEXTS_UNLIKELY(!ltsLS_good(ls))) \
    { \
      ESPAM((LUATEXTS_STRINGIFY(LUATEXTS_CONCAT(ltsLS_readuint, BASE)) \
        ": clipped\n")); \
      return LUATEXTS_ECLIPPED; \
    } \
    LUATEXTS_ENSURE(ls, \
        LUATEXTS_CONCAT(uint_lookup_table_, BASE)[*ls->pos] >= 0, \
        LUATEXTS_EBADDATA, \
        (LUATEXTS_STRINGIFY(LUATEXTS_CONCAT(ltsLS_readuint, BASE)) \
          ": first character is not a number\n") \
      ); \
    while (LUATEXTS_CONCAT(uint_lookup_table_, BASE)[*ls->pos] >= 0) \
    { \
      LUATEXTS_ENSURE(ls, \
          !( \
            (k >= LIMIT) && \
            ( \
              k != LIMIT || \
              LUATEXTS_CONCAT(uint_lookup_table_, BASE)[*ls->pos] > TAIL \
            ) \
          ), \
          LUATEXTS_ETOOHUGE, \
          (LUATEXTS_STRINGIFY(LUATEXTS_CONCAT(ltsLS_readuint, BASE)) \
            ": value does not fit to uint32_t\n") \
        ); \
      k = k * BASE + LUATEXTS_CONCAT(uint_lookup_table_, BASE)[*ls->pos]; \
      EAT_CHAR(ls, LUATEXTS_STRINGIFY(LUATEXTS_CONCAT(ltsLS_readuint, BASE))); \
    } \
    EAT_NEWLINE( \
        ls, LUATEXTS_STRINGIFY(LUATEXTS_CONCAT(ltsLS_readuint, BASE)) \
      ); \
    *dest = k; \
    return LUATEXTS_ESUCCESS; \
  }

DECLARE_READUINT(ltsLS_readuint, 10, 429496729,  5)
DECLARE_READUINT(ltsLS_readuint, 16, 0xFFFFFFF,  0xF)
DECLARE_READUINT(ltsLS_readuint, 36, 119304647,  3)

#undef DECLARE_READUINT

static int ltsLS_readnumber(lts_LoadState * ls, LUATEXTS_NUMBER * dest)
{
  size_t len = 0;
  const unsigned char * data = NULL;
  int result = ltsLS_readline(ls, &data, &len);
  char * endptr = NULL;
  LUATEXTS_NUMBER value = 0;

  if (result != LUATEXTS_ESUCCESS)
  {
    ESPAM(("readnumber: failed to read line"));
    return result;
  }

  LUATEXTS_ENSURE(ls,
      len > 0,
      LUATEXTS_EBADDATA, ("readnumber: empty line instead of number\n")
    );

  /*
  * This is safe, since we're guaranteed to have
  * at least one non-numeric trailing byte.
  */
  value = strtod((const char *)data, &endptr);
  LUATEXTS_ENSURE(ls,
      (const unsigned char *)endptr == data + len,
      LUATEXTS_EGARBAGE, ("readnumber: garbage before eol\n")
    );

  *dest = value;

  return LUATEXTS_ESUCCESS;
}

static int load_value(lua_State * L, lts_LoadState * ls);

/*
* TODO: generalize with load_stream_table.
*/
static int load_fixed_table(lua_State * L, lts_LoadState * ls)
{
  LUATEXTS_UINT array_size = 0;
  LUATEXTS_UINT hash_size = 0;

  LUATEXTS_UINT i = 0;

  int result = ltsLS_readuint10(ls, &array_size);
  if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
  {
    ESPAM(("load_fixed_table: failed to read array size"));
    return result;
  }

  result = ltsLS_readuint10(ls, &hash_size);
  if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
  {
    ESPAM(("load_fixed_table: failed to read hash size"));
    return result;
  }

  LUATEXTS_ENSURE(ls,
      array_size >= 0 && array_size <= MAXASIZE &&
      hash_size >= 0 &&
      (hash_size == 0 || ceillog2((unsigned int)hash_size) <= MAXBITS) &&
      /*
      * Simplification: Assuming minimum value size is one byte.
      */
      ltsLS_unread(ls) >= (array_size + hash_size * 2),
      LUATEXTS_ETOOHUGE, ("load_fixed_table: too huge\n")
    );
  SPAM((
      "load_fixed_table: size: %lu array + %lu hash = %lu total\n",
      array_size, hash_size, array_size + hash_size
    ));

  lua_createtable(L, array_size, hash_size);

  for (i = 0; i < array_size; ++i)
  {
    result = load_value(L, ls); /* Load value. */
    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("load_fixed_table: failed to read array part value\n"));
      return result;
    }

    lua_rawseti(L, -2, i + 1);
  }

  for (i = 0; i < hash_size; ++i)
  {
    int key_type = LUA_TNONE;

    result = load_value(L, ls); /* Load key. */
    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("load_fixed_table: failed to read key\n"));
      return result;
    }

    /* Table key can't be nil or NaN */
    key_type = lua_type(L, -1);
    LUATEXTS_ENSURE(ls,
        key_type != LUA_TNIL &&
        !(key_type == LUA_TNUMBER && luai_numisnan(lua_tonumber(L, -1))),
        LUATEXTS_EBADDATA, ("load_fixed_table: key is nil or nan\n")
      );

    result = load_value(L, ls); /* Load value. */
    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("load_fixed_table: failed to read value\n"));
      return result;
    }

    lua_rawset(L, -3);
  }

  return LUATEXTS_ESUCCESS;
}

/*
* TODO: generalize with load_fixed_table.
*/
static int load_stream_table(lua_State * L, lts_LoadState * ls)
{
  lua_newtable(L);

  while (1) /* We're limited by buffer length */
  {
    int key_type = LUA_TNONE;

    int result = load_value(L, ls); /* Load key. */
    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("load_stream_table: failed to read key\n"));
      return result;
    }

    /* If "key" is nil, this is the end of table (no value expected). */
    key_type = lua_type(L, -1);
    if (key_type == LUA_TNIL)
    {
      lua_pop(L, 1); /* Pop terminating nil */
      break;
    }

    LUATEXTS_ENSURE(ls,
        !(key_type == LUA_TNUMBER && luai_numisnan(lua_tonumber(L, -1))),
        LUATEXTS_EBADDATA, ("load_stream_table: key is nan\n")
      );

    result = load_value(L, ls); /* Load value. */
    if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
    {
      ESPAM(("load_stream_table: failed to read value\n"));
      return result;
    }

    lua_rawset(L, -3);
  }

  return LUATEXTS_ESUCCESS;
}

static int load_value(lua_State * L, lts_LoadState * ls)
{
  size_t len = 0;
  const unsigned char * type = NULL;

  int result = LUATEXTS_ESUCCESS;

  if (LUATEXTS_UNLIKELY(!ltsLS_good(ls)))
  {
    ESPAM(("load_value: clipped\n"));
    return LUATEXTS_ECLIPPED;
  }

  /* Read value type */
  type = ls->pos;

  EAT_CHAR(ls, "ltsLS_readuint10");

  EAT_NEWLINE(ls, "ltsLS_readuint10");

  /* Read value data if any, and push the value to Lua state */
  if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
  {
    SPAM((
        "load_value: value type '%c' 0x%X (%d)\n",
        isgraph(*type) ? *type : '?', *type, *type
      ));

    luaL_checkstack(L, 1, "load-value");

    switch (*type)
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
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            lua_pushnumber(L, value);
          }
        }
        break;

      case LUATEXTS_CUINT:
        {
          LUATEXTS_UINT value;

          result = ltsLS_readuint10(ls, &value);
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            /* TODO: Maybe do lua_pushinteger if value fits? */
            lua_pushnumber(L, value);
          }
        }
        break;

      case LUATEXTS_CUINTHEX:
        {
          LUATEXTS_UINT value;

          result = ltsLS_readuint16(ls, &value);
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            /* TODO: Maybe do lua_pushinteger if value fits? */
            lua_pushnumber(L, value);
          }
        }
        break;

      case LUATEXTS_CUINT36:
        {
          LUATEXTS_UINT value;

          result = ltsLS_readuint36(ls, &value);
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            /* TODO: Maybe do lua_pushinteger if value fits? */
            lua_pushnumber(L, value);
          }
        }
        break;

      case LUATEXTS_CSTRING:
        {
          const unsigned char * str = NULL;

          /* Read string size */
          LUATEXTS_UINT len = 0;
          result = ltsLS_readuint10(ls, &len);

          /* Check size */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            /* Implementation detail */
            if (LUATEXTS_UNLIKELY((lua_Integer)len < 0))
            {
              ESPAM(("string: value does not fit to lua_Integer\n"));
              ltsLS_close(ls);
              result = LUATEXTS_ETOOHUGE;
            }
          }

          /* Read string data */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            str = ltsLS_eat(ls, len);
            if (LUATEXTS_UNLIKELY(str == NULL))
            {
              ESPAM(("load_value: bad string size\n"));
              ltsLS_close(ls);
              result = LUATEXTS_EBADSIZE;
            }
          }

          /* Eat newline after string data */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            size_t empty = 0;
            const unsigned char * nl = NULL;
            result = ltsLS_readline(ls, &nl, &empty);
            if (
                LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS)
                  && LUATEXTS_UNLIKELY(empty != 0)
              )
            {
              ESPAM(("load_value: string: garbage before eol\n"));
              ltsLS_close(ls);
              result = LUATEXTS_EGARBAGE;
            }
          }

          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
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
          result = ltsLS_readuint10(ls, &len_chars);

          /* Check size */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            /* Implementation detail */
            if (LUATEXTS_UNLIKELY((lua_Integer)len < 0))
            {
              ESPAM(("stringutf8: value does not fit to lua_Integer\n"));
              ltsLS_close(ls);
              result = LUATEXTS_ETOOHUGE;
            }
          }

          /* Read string data */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            result = ltsLS_eatutf8(ls, len_chars, &str, &len_bytes);
          }

          /* Eat newline after string data */
          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            size_t empty = 0;
            const unsigned char * nl = NULL;
            result = ltsLS_readline(ls, &nl, &empty);
            if (
                LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS)
                  && LUATEXTS_UNLIKELY(empty != 0)
              )
            {
              ESPAM(("load_value: utf8: garbage before eol\n"));
              ltsLS_close(ls);
              result = LUATEXTS_EGARBAGE;
            }
          }

          if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
          {
            lua_pushlstring(L, (const char *)str, len_bytes);
          }
        }
        break;

      case LUATEXTS_CFIXEDTABLE:
        result = load_fixed_table(L, ls);
        break;

      case LUATEXTS_CSTREAMTABLE:
        result = load_stream_table(L, ls);
        break;

      default:
        ESPAM(("load_value: unknown type char 0x%X (%d)\n", type[0], type[0]));
        ltsLS_close(ls);
        result = LUATEXTS_EBADTYPE;
        break;
    }
  }

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

  result = ltsLS_readuint10(&ls, &tuple_size);
  /* Check size */
  if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
  {
    /* Implementation detail */
    if (LUATEXTS_UNLIKELY((lua_Integer)tuple_size < 0))
    {
      ESPAM(("tuple: size does not fit to lua_Integer\n"));
      ltsLS_close(&ls);
      result = LUATEXTS_ETOOHUGE;
    }
  }

  for (i = 0; i < tuple_size && result == LUATEXTS_ESUCCESS; ++i)
  {
    SPAM(("load_tuple: loading value %lu of %lu\n", i + 1, tuple_size));
    result = load_value(L, &ls);
  }

  /*
  * Security note: ignoring unread bytes if any.
  * Motivation: more casual, this is a text format after all.
  */

  if (LUATEXTS_LIKELY(result == LUATEXTS_ESUCCESS))
  {
    *count = tuple_size;
  }
  else
  {
    XESPAM(("load_tuple: error %d\n", result));

    lua_settop(L, base); /* Discard intermediate results */

    luaL_checkstack(L, 1, "load-err");

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

  luaL_checkstack(L, 1, "lload");
  lua_pushboolean(L, 1);

  result = luatexts_load(L, buf, len, &tuple_size);
  if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
  {
    luaL_checkstack(L, 1, "lload-err");
    lua_pushnil(L);
    lua_replace(L, -3); /* Replace pre-pushed true with nil */
    return 2; /* Error message already on stack */
  }

  return tuple_size + 1;
}

/* TODO: Hide this mmap stuff in a separate file */
/* TODO: Support fd as an argument instead of a filename */
/*
* TODO: Not quite exception-safe.
*       Must put close() and unmap() calls to __gc somewhere,
*       so they would be called on error.
*/
static int lload_from_file(lua_State * L)
{
  const char * filename = (const char *)luaL_checkstring(L, 1);

  size_t tuple_size = 0;
  int result = 0;

  const unsigned char * buf = NULL;

  struct stat sb;

  int fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    luaL_checkstack(L, 2, "lloadff-err");
    lua_pushnil(L);
    lua_pushfstring(
        L, "load_from_file failed: can't open " LUA_QL("%s") " for reading: %s",
        filename, strerror(errno)
      );
    return 2;
  }

  if (fstat(fd, &sb) == -1)
  {
    luaL_checkstack(L, 2, "lloadff-err");
    lua_pushnil(L);
    lua_pushfstring(
        L, "load_from_file failed: can't stat " LUA_QL("%s") ": %s",
        filename, strerror(errno)
      );
    close(fd);
    return 2;
  }

  if (!S_ISREG(sb.st_mode))
  {
    luaL_checkstack(L, 2, "lloadff-err");
    lua_pushnil(L);
    lua_pushfstring(
        L, "load_from_file failed: " LUA_QL("%s") " is not a file",
        filename
      );
    close(fd);
    return 2;
  }

  if (sb.st_size == 0)
  {
    luaL_checkstack(L, 2, "lloadff-err");
    lua_pushnil(L);
    lua_pushfstring(
        L, "load_from_file failed: " LUA_QL("%s") " is empty",
        filename
      );
    close(fd);
    return 2;
  }

  buf = (const unsigned char *)mmap(
      0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0
    );
  if (buf == MAP_FAILED)
  {
    luaL_checkstack(L, 2, "lloadff-err");
    lua_pushnil(L);
    lua_pushfstring(
        L, "load_from_file failed: " LUA_QL("%s") " mmap failed: %s",
        filename, strerror(errno)
      );
    close(fd);
    return 2;
  }

  close(fd);

  luaL_checkstack(L, 1, "lloadff");
  lua_pushboolean(L, 1);

  result = luatexts_load(L, buf, sb.st_size, &tuple_size);
  if (LUATEXTS_UNLIKELY(result != LUATEXTS_ESUCCESS))
  {
    luaL_checkstack(L, 1, "lloadff-err");
    lua_pushnil(L);
    lua_replace(L, -3); /* Replace pre-pushed true with nil */
    return 2; /* Error message already on stack */
  }

  if (munmap((void *)buf, sb.st_size) == -1)
  {
    ESPAM(("lloadff: munmap failed"));
    /* What else can we do? */
  }

  return tuple_size + 1;
}

/* Lua module API */
static const struct luaL_reg R[] =
{
  { "load", lload },
  { "load_from_file", lload_from_file },

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
