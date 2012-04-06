--------------------------------------------------------------------------------
-- filesystem.lua: shell-dependant code to work with files and directories
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local os = os

local shell_exec
      = import 'lua-aplicado/shell.lua'
      {
        'shell_exec'
      }

--------------------------------------------------------------------------------
local copy_file_to_dir = function(filename, dir)
  assert(shell_exec(
      "cp", filename, dir .. "/"
    ) == 0)
end
-------------------------------------------------------------------------------
local remove_file = function(filename)
  assert(os.remove(filename))
end
-------------------------------------------------------------------------------
local remove_recursively = function(filename, force)
  local args = force and "-rf" or "-r"
  assert(shell_exec(
      "rm", args, filename
    ) == 0)
end
-------------------------------------------------------------------------------
local create_symlink_from_to = function(from_filename, to_filename)
  assert(shell_exec(
      "ln", "-s", from_filename, to_filename
    ) == 0)
end
-------------------------------------------------------------------------------
local copy_file = function(filename, new_filename)
  assert(shell_exec(
      "cp", filename, new_filename
    ) == 0)
end
-------------------------------------------------------------------------------
local copy_file_with_flag = function(filename, new_filename, flag)
  assert(shell_exec(
      "cp", flag, filename, new_filename
    ) == 0)
end
-------------------------------------------------------------------------------
return
{
  copy_file_to_dir = copy_file_to_dir;
  remove_file = remove_file;
  remove_recursively = remove_recursively;
  create_symlink_from_to = create_symlink_from_to;
  copy_file = copy_file;
  copy_file_with_flag = copy_file_with_flag;
}
