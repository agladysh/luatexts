--------------------------------------------------------------------------------
-- remote.lua: dumb ad-hoc code to work with sh-like shells remotely
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local assert
    = assert

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

local shell_exec,
      shell_read,
      shell_exec_no_subst,
      shell_read_no_subst,
      shell_format_command,
      shell_format_command_no_subst
      = import 'lua-aplicado/shell.lua'
      {
        'shell_exec',
        'shell_read',
        'shell_exec_no_subst',
        'shell_read_no_subst',
        'shell_format_command',
        'shell_format_command_no_subst'
      }

--------------------------------------------------------------------------------

local shell_exec_remote = function(host, ...)
  return shell_exec_no_subst(
      "ssh", host, shell_format_command(...)
    )
end

local shell_read_remote = function(host, ...)
  return shell_read_no_subst(
      "ssh", host, shell_format_command(...)
    )
end

--------------------------------------------------------------------------------

return
{
  shell_exec_remote = shell_exec_remote;
  shell_read_remote = shell_read_remote;
}
