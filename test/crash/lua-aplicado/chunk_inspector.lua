--------------------------------------------------------------------------------
-- chunk_inspector.lua: basic Lua chunk inspector with support for LJ2
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
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

local running_under_luajit
      = import 'lua-aplicado/luajit2.lua'
      {
        'running_under_luajit'
      }

--------------------------------------------------------------------------------

local make_chunk_inspector

if not running_under_luajit() then

  make_chunk_inspector = import 'lua-aplicado/bci_chunk_inspector.lua'
  {
    'make_chunk_inspector'
  }

else

  make_chunk_inspector = import 'lua-aplicado/lj2_chunk_inspector.lua'
  {
    'make_chunk_inspector'
  }

end

--------------------------------------------------------------------------------

return
{
  make_chunk_inspector = make_chunk_inspector;
}
