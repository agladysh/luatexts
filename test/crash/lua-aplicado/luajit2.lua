--------------------------------------------------------------------------------
-- luajit2.lua: LJ2 detection
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

declare 'jit' -- Hack
local jit = jit

--------------------------------------------------------------------------------

local arguments,
      optional_arguments,
      method_arguments,
      eat_true
      = import 'lua-nucleo/args.lua'
      {
        'arguments',
        'optional_arguments',
        'method_arguments',
        'eat_true'
      }

--------------------------------------------------------------------------------

local running_under_luajit = function()
  return (jit ~= nil)
end

--------------------------------------------------------------------------------

return
{
  running_under_luajit = running_under_luajit;
}
