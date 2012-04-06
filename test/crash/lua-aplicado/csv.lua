--------------------------------------------------------------------------------
-- csv.lua: basic code to convert csv-data to lua table
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--
-- Based on MIT-licensed[1] csv.lua[2] from LuaPlus[3].
-- [1] https://github.com/jjensen/luaplus51-all/blob/ac2808f1192aa435e7e3c4fb5789414256b4caf9/License.txt
-- [2] https://github.com/jjensen/luaplus51-all/blob/ac2808f1192aa435e7e3c4fb5789414256b4caf9/Src/Modules/lexers/csv.lua
-- [3] https://github.com/jjensen/luaplus51-all
-- NOTE: changed separator "," to ";"
--------------------------------------------------------------------------------

local arguments,
      optional_arguments,
      method_arguments
      = import 'lua-nucleo/args.lua'
      {
        'arguments',
        'optional_arguments',
        'method_arguments'
      }

local lpeg = require 'lpeg'

------------------------------------------------------------------------

-- convert csv data to lua table
local csv_convert_to_table

do
  local C, Cs, Ct, P, S = lpeg.C, lpeg.Cs, lpeg.Ct, lpeg.P, lpeg.S

  local eol = P'\r\n' + P'\n'
  local quoted_field = '"' * Cs(((P(1) - '"') + P'""' / '"')^0) * '"'
  local unquoted_field = C((1 - S';\r\n"')^0)
  local field = quoted_field + unquoted_field
  local record = Ct(field * (';' * field)^0)
  local nonemptyrecord = #P(1 - eol) * record
  local records = Ct((record * eol)^0 * nonemptyrecord^-1) * -1
  local one_line_record = record * (eol + -1)

  csv_convert_to_table = function(input, ...)
    arguments(
        "string", input
      )
    return lpeg.match(records, input)
  end
end
------------------------------------------------------------------------

return
{
  csv_convert_to_table = csv_convert_to_table;
}
