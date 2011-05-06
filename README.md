luatexts: Trivial Lua human-readable binary-safe serialization library
======================================================================

See the copyright information in the file named `COPYRIGHT`.

Why not...
----------

* XML — large and unwieldy
* JSON — not Lua-aware (can't encode table-as-key and other fancy values)
* Lua code — needs sandboxing, slower load times
* Luabins — is too binary for some languages
* ...

Inspired by
-----------

* Luabins: http://github.com/agladysh/luabins/
* Redis transport protocol format: http://redis.io/topics/protocol

Supported data types
--------------------

* `nil`
* `boolean`
* `number`
* `string`
* `table` (no references)

Format
------

### Note on human-readability

Note that while human-readability is one of design goals,
nobody claims that the actual data is easy to read or write by hand.

You can do it, but you're better off using some API instead.

If you often need to work with luatexts data manually for purposes
other than debugging, consider using something more friendly like
JSON or XML or sandboxed Lua code.

### Definition

The luatexts format is defined as follows:

    <unsigned-integer-data:tuple-size>\n
    <type1>\n
    <data1>\n
    ...
    <typeN>\n
    <dataN>\n

*`\n` here and below may be either `LF` or `CRLF`*

### Types

* Nil
  * type: `-`
  * data: *(none)*
* Boolean (false)
  * type: `0`
  * data: *(none)*
* Boolean (true)
  * type: `1`
  * data: *(none)*
* Number (double)
  * type: `N`
  * data: *plain string representation, readable by `strtod`*
* Number (unsigned integer)
  * type: `U`
  * data: *plain string representation, readable by `strtoul`*
* String (regular)
  * type: `S`
  * data:

              <unsigned-data:size-in-bytes>\n
              <string-data, "binary" stuff supported>

* String (UTF-8)
  * type: `8`
  * data:

              <unsigned-data:length-in-characters>\n
              <string-data, only valid UTF-8 supported, without BOM>

* Table
  * type: `T`
  * data:

              <unsigned-data:array-size>\n
              <unsigned-data:hash-size>\n
              <array-item-1>\n
              ...
              <array-item-N>\n
              <hash-key-1>\n
              <hash-value-1>\n
              ...
              <hash-key-N>\n
              <hash-value-N>

### Notes on table data type:

* Nested tables are supported;
* references in tables are not supported (saved as separate tables);
* if table references itself, it can not be saved;
* array/hash separation is optional, encoder can opt to use hash part only,
  (but decoder must support both);
* array part may include `nil` values (hash values may be `nil` as well);
* table keys may not be `NaN` or `nil`.

### Examples

*Everything on the line after `\n` is comments, do not put them into your data.*

* Zero-sized tuple

  In Lua:

        return

  In luatexts:

        0\n ; == Tuple size ==

* Mutiple values in tuple

  In Lua:

        return 42, "Hello, world!\n", true

  In luatexts:

        3\n               ; == Tuple size ==
        N\n               ; -- Number --
        42\n              ; Number value
        S\n               ; -- String --
        13\n              ; String size in bytes
        Hello, world!\n\n ; String data
        1\n               ; -- Boolean true --

* Simple table

  In Lua:

        return { 42 }

  In luatexts:

        1\n  ; == Tuple size ==
        T\n  ; -- Table --
        1\n  ; Array part size
        0\n  ; Hash part size
        N\n  ; [1]: -- Number --
        42\n ; [1]: Number value

  In luatexts (equivalent):

        1\n  ; == Tuple size ==
        T\n  ; -- Table --
        0\n  ; Array part size
        1\n  ; Hash part size
        N\n  ; Key: -- Number --
        1\n  ; Key: Number value
        N\n  ; Value: -- Number --
        42\n ; Value: Number value

### Array vs. hash parts of a table

Note that, as far as Lua implementation is concerned,
it does not matter much if you put a value in array part or in hash part
of a table in your luatexts data.

As in Lua table constructor, you can even put a value for the same key both
in array and in hash part of a table:

In Lua:

      return { 3.14, [1] = 2.71 }

In luatexts:

      1\n    ; == Tuple size ==
      T\n    ; -- Table --
      1\n    ; Array part size
      1\n    ; Hash part size
      N\n    ; [1]: -- Number --
      3.14\n ; [1]: Number value
      N\n    ; Key: -- Number --
      1\n    ; Key: Number value
      N\n    ; Value: -- Number --
      2.71\n ; Value: Number value

As in Lua, it is not defined which one of values would end up
in the loaded table.

### Regular strings vs. UTF-8 strings

Regular strings are treated just as a blob of bytes. Their size
is specified in *bytes*, and reader implementation never looks
inside the string data. You can pass any data, including something binary
as a regular string. (Obviously, you can also pass UTF-8 strings
as regular strings.)

UTF-8 strings are honestly treated as UTF-8 data. Their size is
specified in *characters*, and reader implementation does read UTF-8
characters one-by-one, doing the full validation. You can only pass valid
UTF-8 text data as an UTF-8 string.

Apart from validation, in Lua implementation it does not matter much,
if you used regular string or UTF-8 string type to represent your data.
But UTF-8 strings are handy if you compose your data payload from the
language that does not know anything about bytes.

Security notes
--------------

Protect from memory attacks by limiting the payload string size.
Luatexts does not do that for you.

Mini-FAQ
--------

1. Why no `save()` in Lua and no `load()` in other language versions?

    Did not have time to write them yet. Do not need them personally,
    because I always try to feed the data to the consumer
    in the format consumer understands best.

2. What if you need one of these missing functions?

    * Use luabins or other feature-complete serialization library.
    * Write it yourself (it is easy!) and send me a pull request.
    * Ask me nicely.

3. When to use luatexts and when luabins?

   * Use either of them when data consumer is Lua code.
     Outside of that both formats are of limited usefulness,
     since they follow Lua-specific data semantics closely.

   * If data producer language does have luabins or luatexts bindings, use them.
     If both are available, prefer luabins for speed,
     luatexts for human-readability.

   * If data producer language does not have existing bindings
     and it is  C-aware, use luabins, as it has Lua-less C serialization API.
     I would appreciate if you would share your bindings code with
     the community, but it is not mandatory.

   * Otherwise, if data producer language is not C-aware, use luatexts,
     as luatexts format writer it is more trivial to implement
     (see JS API for an example). Again, I would greatly appreciate if you
     will share your implementation with the community,
     but it is not mandatory.

API
---

### Lua (C API)

    local luatexts = require 'luatexts'

* `luatexts.load(data : string) : true, ... / nil, err`

  Returns unserialized data tuple (as multiple return values).
  Tuples may be of zero values.

### Lua (Plain)

This module is primarily used in tests. It may be considered as a reference
implementation of luatexts data serializer.

    local luatexts_lua = require 'luatexts.lua'

* `luatexts_lua.save(...) : string / nil, err`

  Serializes given data tuple. Returns `nil, err` on error.

  Issues:

  * Throws `error()` on self-referencing tables.
  * Asserts if detects non-serializable value inside a table.

  (Both issues to be fixed in later revisions.)

### JavaScript

* `LUATEXTS.save(...) : string`

  Serializes its arguments and returns them as a string.
  If does not know how to serialize value, throws `Exception`.
  Call without arguments produces a zero-sized tuple.

Type conversion rules for JS --> Lua:

* `undefined` --> `nil`
* `null` --> `nil`
* `boolean` --> `boolean`
* `number` --> `number`
* `string` --> `string` (assuming UTF-8 encoding)
* `object` --> `table` with hash part
* `array` --> `table` with array part (implicitly saved as 1-based)
* `function` --> not supported

Nested objects / arrays are supported.

**Warning:** For JavaScript client code to work consistently between browsers,
you must specify the encoding of the page that the code is executed in.
Otherwise each browser will assume its own default encoding.
Make sure that the encoding does match whatever your server-side code
does expect.

To specify the encoding you may either send correct `Content-Encoding`
HTTP header, or put this tag into the page's `<head>`:

    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

Current JavaScript library implementation works *only* with UTF-8 strings.
It is up to user to ensure that string encoding is correct.

### PHP

* `Luatexts::save( ... ) : string`

  Serializes its arguments and returns them as a string.
  If does not know how to serialize value, throws `Exception`.
  Call without arguments produces a zero-sized tuple.

Type conversion rules for JS --> Lua:

* `null` --> `nil`
* `boolean` --> `boolean`
* `number` --> `number`
* `string` --> `string` (assuming UTF-8 encoding)
* `array` --> `table` with array part (implicitly saved as 1-based)
* `function` --> not supported
* `object` --> not supported

Nested arrays are supported.