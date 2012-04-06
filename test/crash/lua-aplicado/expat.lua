--------------------------------------------------------------------------------
-- expat.lua: basic code to convert lxp.lom object to table with tags as a keys
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local unique_object
      = import 'lua-nucleo/misc.lua'
      {
        'unique_object'
      }

local trim
      = import 'lua-nucleo/string.lua'
      {
        'trim'
      }

local arguments,
      optional_arguments,
      method_arguments
      = import 'lua-nucleo/args.lua'
      {
        'arguments',
        'optional_arguments',
        'method_arguments'
      }

------------------------------------------------------------------------

local LOM_ATTRS = unique_object()
local xml_convert_lom
do
  --reads a list of attributes from a intger-index part of source table and
  -- return the table of the form "attribute name" => "value"
  --example:
  --source table: attr={"id","status","sum",status="150",sum="0.05",id="FF00"}
  --result: {status="150",sum="0.05",id="FF00"}
  local xml_convert_attrs = function(src)
    local dest = { }

    for i = 1, #src do
      dest[src[i]] = src[src[i]]
    end

    return dest
  end

  local function impl(t, visited)
    arguments(
        "table", t,
        "table", visited
      )

    local result = { }

    assert(t.attr, "missing attr field")
    result[LOM_ATTRS] = xml_convert_attrs(t.attr)

    for i = 1, #t do
      local v = t[i]
      if type(v) ~= "table" then
        result['value'] = result['value'] or ""
        result['value'] = result['value'] .. trim(v)
      else
        assert(v.tag, "missing tag name")
        result[v.tag] = result[v.tag] or { }

        assert(not visited[v], "recursion detected")
        visited[v] = true
        result[v.tag][#result[v.tag] + 1] = impl(v, visited)
        visited[v] = nil
      end
    end

    return result
  end

  xml_convert_lom = function(t)
    local r = { }

    assert(t.tag, "missing tag name")
    r[t.tag] = impl(t, { })

    return r
  end
end

------------------------------------------------------------------------

return
{
  LOM_ATTRS = LOM_ATTRS;
  xml_convert_lom = xml_convert_lom;
}
