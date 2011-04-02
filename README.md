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

Supported data types
--------------------

* `nil`
* `boolean`
* `number`
* `string`
* `table` (no references)

Format
------

    <unsigned-integer-data:tuple-size>\n<type1>\n<data1>\n...<typeN>\n<dataN>\n

*`\n` here and below may be either `LF` or `CRLF`*

* Nil
  * type: `-`
  * data: *(none)*
* Boolean-false
  * type: `0`
  * data: *(none)*
* Boolean-true
  * type: `1`
  * data: *(none)*
* Number-double
  * type: `N`
  * data: *plain string representation, readable by `strtod`*
* Number-unsigned-integer
  * type: `U`
  * data: *plain string representation, readable by `strtoul`*
* String-any
  * type: `S`
  * data:
    <unsigned-data:size-in-bytes>\n
    <string-data, "binary" stuff supported>
* String-UTF-8
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

Notes on table data type:

* nested tables are supported;
* references in tables are not supported (saved as separate tables);
* if table references itself, it can not be saved;
* array/hash separation is optional, encoder can opt to use hash part only,
  (but decoder must support both);
* array part may include `nil` values (hash values may be `nil` as well);
* table keys may not be `NaN` or `nil`.

Security notes
--------------

Protect from memory attacks by limiting the payload string size.
Luatexts does not do that for you.

Mini-FAQ
--------

1. Why no `save()` in Lua and no `load()` in other language versions?

Did not have time to write them yet. Do not need them personally,
because I always try to feed the data to the consumer code
in the format consumer understands best.

2. What if you need one of these missing functions?

    * Use luabins or other feature-complete serialization library.
    * Write it yourself (it is easy!) and send me a pull request.
    * Ask me nicely.

Inspired by
-----------

* Luabins
* Redis transport protocol format

API
---

### Lua

* `luatexts.load(data : string) : true, ... / nil, err`

  Returns unserialized data tuple (as multiple return values).
  Tuples may be of zero values.

### JavaScript

* `LUATEXTS.save(...) : string`

  Serializes its arguments and returns them as a string.
  If does not know how to serialize value, throws `Exception`.
  Call without arguments produces a zero-sized tuple.

Type conversion rules for JS->Lua:

* `undefined` --> `nil`
* `null`: --> `nil`
* `boolean`: `boolean`
* `number`: `number`
* `string`: `string` (assuming UTF-8 encoding)
* `object`: `table` with hash part
* `array`: `table` with array part (implicitly saved as 1-based)
* `function`: not supported

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
