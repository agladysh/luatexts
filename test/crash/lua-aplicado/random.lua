--------------------------------------------------------------------------------
-- random.lua: utilities for random generation
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

require 'md5'

local arguments,
      optional_arguments,
      method_arguments
      = import 'lua-nucleo/args.lua'
      {
        'arguments',
        'optional_arguments',
        'method_arguments'
      }

--------------------------------------------------------------------------------

-- generates pseudorandom number based on input string
local random_seed_from_string = function(base_string)
  base_string = base_string or tostring(os.time())
  arguments(
      "string", base_string
    )
  -- random seed length is limited by 7 hexadecimal digits to match RAND_MAX
  return tonumber(string.sub(md5.sumhexa(base_string), 1, 7), 16)
end

--------------------------------------------------------------------------------

return
{
  random_seed_from_string = random_seed_from_string;
}
