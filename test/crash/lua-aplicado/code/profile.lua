--------------------------------------------------------------------------------
-- profile.lua: lua-aplicado exports profile
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local tset = import 'lua-nucleo/table-utils.lua' { 'tset' }

--------------------------------------------------------------------------------

local PROFILE = { }

--------------------------------------------------------------------------------

PROFILE.skip = setmetatable(tset
{
  "lua-aplicado/module.lua";  -- Too low-level

  -- Excluding both low-level chunk inspector implementations
  -- Higher-level implementation would pick proper one.
  "lua-aplicado/lj2_chunk_inspector.lua";
  "lua-aplicado/bci_chunk_inspector.lua";
}, {
  __index = function(t, k)
    -- Excluding files outside of lua-aplicado/ and inside lua-aplicado/code
    local v = (not k:match("^lua%-aplicado/")) or k:match("^lua%-aplicado/code/")
    t[k] = v
    return v
  end;
})

--------------------------------------------------------------------------------

return PROFILE
