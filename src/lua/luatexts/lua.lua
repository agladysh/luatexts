--------------------------------------------------------------------------------
-- luatexts/lua.lua: plain Lua implementation of luatexts
--------------------------------------------------------------------------------
-- Copyright (c) 2011, luatexts authors
-- See license in the file named COPYRIGHT
--------------------------------------------------------------------------------

local assert, error, pairs, select, type
    = assert, error, pairs, select, type

local table_concat
    = table.concat

--------------------------------------------------------------------------------

local save
do
  local handlers = { }

  local handle_value = function(cat, v, visited, buf)
    local handler = handlers[type(v)]
    if handler == nil then
      return nil, "can't save `" .. type(v) .. "'"
    end
    return handler(cat, v, { }, buf)
  end

  handlers["nil"] = function(cat, v, visited, buf)
    return cat "-" "\n"
  end

  handlers["boolean"] = function(cat, v, visited, buf)
    return cat (v and "1" or "0") "\n"
  end

  handlers["number"] = function(cat, v, visited, buf)
    return cat "N" "\n" (("%.54g"):format(v)) "\n"
  end

  handlers["string"] = function(cat, v, visited, buf)
    return cat "S" "\n" (#v) "\n" (v) "\n"
  end

  handlers["table"] = function(cat, t, visited, buf)
    if visited[t] then
      -- TODO: This should be `return nil, err`, not `error()`!
      error("circular table reference detected")
    end
    visited[t] = true

    cat "T" "\n"

    local array_size = #t
    cat (array_size) "\n"

    local hash_size_pos = #buf + 1
    cat ("?") "\n"

    for i = 1, array_size do
      handle_value(cat, t[i], visited, buf)
    end

    local hash_size = 0
    for k, v in pairs(t) do
      if
        type(k) ~= "number" or
        k > array_size or k < 1 or -- integer key in hash part of the table
        k % 1 ~= 0 -- non-integer key
      then
        hash_size = hash_size + 1
        -- TODO: return nil, err on failure instead of asserting
        assert(handle_value(cat, k, visited, buf))
        assert(handle_value(cat, v, visited, buf))
      end
    end

    buf[hash_size_pos] = hash_size

    visited[t] = nil

    return cat
  end

  save = function(...)
    local nargs = select("#", ...)
    local buf = { }
    local function cat(s) buf[#buf + 1] = s; return cat end

    cat (nargs) "\n"

    for i = 1, nargs do
      handle_value(cat, select(i, ...), { }, buf)
    end

    return table_concat(buf)
  end
end

--------------------------------------------------------------------------------

return
{
  _VERSION = "luatexts-lua 0.1.1";
  _COPYRIGHT = "Copyright (C) 2011, luatexts authors";
  _DESCRIPTION = "Trivial Lua human-readable binary-safe serialization library";
  --
  save = save;
  -- Sorry, no load() (yet). Patches are welcome.
}
