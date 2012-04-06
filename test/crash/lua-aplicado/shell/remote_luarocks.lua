--------------------------------------------------------------------------------
-- shell/remote_luarocks.lua: dumb ad-hoc code to work with luarocks remotely
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------
-- TODO: Generalize with shell/luarocks.lua!
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

local shell_exec_remote,
      shell_read_remote
      = import 'lua-aplicado/shell/remote.lua'
      {
        'shell_exec_remote',
        'shell_read_remote'
      }

--------------------------------------------------------------------------------

local remote_luarocks_remove_forced = function(host, rock_name)
  assert(shell_exec_remote(
      host, "sudo", "luarocks", "remove", "--force", rock_name
    ) == 0)
end

local remote_luarocks_ensure_rock_not_installed_forced = function(host, name)
  shell_exec_remote(
      host, "sudo", "luarocks", "remove", "--force", name
    ) -- Ignoring errors
end

local remote_luarocks_install_from = function(host, rock_name, rocks_repo)
  assert(shell_exec_remote(
      host, "sudo", "luarocks", "install", rock_name, "--only-from="..rocks_repo
    ) == 0)
end

local remote_luarocks_list_installed_rocks = function(host)
  return shell_read_remote(host, "sudo", "luarocks", "list")
end

--------------------------------------------------------------------------------

return
{
  remote_luarocks_remove_forced = remote_luarocks_remove_forced;
  remote_luarocks_ensure_rock_not_installed_forced = remote_luarocks_ensure_rock_not_installed_forced;
  remote_luarocks_install_from = remote_luarocks_install_from;
  remote_luarocks_list_installed_rocks = remote_luarocks_list_installed_rocks;
}
