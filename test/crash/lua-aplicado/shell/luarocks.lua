--------------------------------------------------------------------------------
-- shell/luarocks.lua: dumb ad-hoc code to work with luarocks
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local assert, loadfile
    = assert, loadfile

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

local tkeys
      = import 'lua-nucleo/table-utils.lua'
      {
        'tkeys'
      }

local do_in_environment
      = import 'lua-nucleo/sandbox.lua'
      {
        'do_in_environment'
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

local tdeepequals
      = import 'lua-nucleo/tdeepequals.lua'
      {
        'tdeepequals'
      }

local is_table,
      is_string
      = import 'lua-nucleo/type.lua'
      {
        'is_table',
        'is_string'
      }

local trim =
      import "lua-nucleo/string.lua"
      {
        'trim'
      }

local find_all_files
      = import 'lua-aplicado/filesystem.lua'
      {
        'find_all_files'
      }

--------------------------------------------------------------------------------

local luarocks_exec = function(...)
  return shell_exec("sudo", "luarocks", ...)
end

local luarocks_read = function(...)
  return shell_read("sudo", "luarocks", ...)
end

local luarocks_read_no_sudo = function(...)
  return shell_read("luarocks", ...)
end

local luarocks_show_rock_dir = function(...)
  return trim(luarocks_read_no_sudo("show", "--rock-dir", ...))
end

local luarocks_exec_no_sudo = function(...)
  return shell_exec("luarocks", ...)
end

local luarocks_exec_dir = function(dir, ...)
  return shell_exec(
      "cd", dir,
      "&&", "sudo", "luarocks", ...
    )
end

local luarocks_exec_dir_no_sudo = function(dir, ...)
  return shell_exec(
      "cd", dir,
      "&&", "luarocks", ...
    )
end

local luarocks_admin_exec_dir = function(dir, ...)
  return shell_exec(
      "cd", dir,
      "&&", "luarocks-admin", ...
    )
end

local luarocks_remove_forced = function(rock_name)
  assert(luarocks_exec(
      "remove", "--force", rock_name
    ) == 0)
end

local luarocks_ensure_rock_not_installed_forced = function(rock_name)
  luarocks_exec(
      "remove", "--force", rock_name
    ) -- Ignoring errors
end

local luarocks_make_in = function(rockspec_filename, path)
  assert(luarocks_exec_dir(
      path, "make", rockspec_filename
    ) == 0)
end

local luarocks_pack_to = function(rock_name, rocks_repo_path)
  assert(luarocks_exec_dir_no_sudo(
      rocks_repo_path, "pack", rock_name
    ) == 0)
end

local luarocks_admin_make_manifest = function(rocks_repo_path)
  assert(luarocks_admin_exec_dir(
      rocks_repo_path, "make-manifest", "."
    ) == 0)
end

local luarocks_load_manifest = function(filename)
  local manifest_chunk = assert(loadfile(filename))

  local env = { }
  local ok, result = assert(
      do_in_environment(
          manifest_chunk,
          env
        )
    )
  assert(result == nil)

  return env
end

local luarocks_get_rocknames_in_manifest = function(filename)
  return tkeys(assert(luarocks_load_manifest(filename).repository))
end

local luarocks_install_from = function(rock_name, rocks_repo)
  assert(shell_exec(
      "sudo", "luarocks", "install", rock_name, "--only-from="..rocks_repo
    ) == 0)
end

local luarocks_parse_installed_rocks = function(list_str)
  local installed_rocks_set, duplicate_rocks_set = { }, { }

  local mode = "installed_rocks"

  local rock_name
  for line in list_str:gmatch("(.-)\n") do
    line:gsub("%s+$", ""):gsub("^%s+", "") -- Trim line

    -- log("luarocks list:", line)

    if mode == "installed_rocks" then
      if line == "" then -- skip initial newlines
        mode = "installed_rocks"
      else
        assert(line == "Installed rocks:")
        mode = "dashed_line"
      end
    elseif mode == "dashed_line" then
      assert(line == "----------------")
      mode = "empty_line"
    elseif mode == "empty_line" then
      assert(line == "")
      mode = "rock_name"
    elseif mode == "rock_name" then
      assert(line:sub(1, 1) ~= " ")
      rock_name = line
      installed_rocks_set[rock_name] = true
      mode = "rock_version_first"
    elseif mode == "rock_version_first" then
      assert(line:sub(1, 1) == " ")
      mode = "duplicate_rock_version_or_empty_line"
    elseif mode == "duplicate_rock_version_or_empty_line" then
      if line == "" then
        rock_name = nil
        mode = "rock_name"
      else
        duplicate_rocks_set[assert(rock_name)] = true
        assert(line:sub(1, 1) == " ")
        mode = "duplicate_rock_version_or_empty_line"
      end
    end
  end

  return installed_rocks_set, duplicate_rocks_set
end

local luarocks_load_rockspec = function(filename)
  local env = { }
  local ok, result = do_in_environment(
      assert(
          loadfile(filename)
        ),
      env
    )
  assert(result == nil)
  return env
end

-- Function creates list of all files mentioned in "build" subtable of .rockspec
-- IN:
-- filename: absolute path to .rockspec file
-- current_dir: root directory of rocks sources
-- OUT:
-- list of files luarocks will be probably use to make .rock
-- List is generated based on all keys of "build" subtable of .rockspec
-- that contain filenames or directory names.
local luarocks_list_rockspec_files
do
  -- recursive table scanning here
  local go_through
  local function impl(t, dirs, fs, current_dir, visited)
  arguments(
      "table", t,
      "table", dirs,
      "table", fs,
      "string", current_dir,
      "table", visited
    )
    for k, v in pairs(t) do
      if is_table(v) then
        assert(not visited[v], "recursion detected")
        visited[v] = true
        dirs, fs = impl(v, dirs, fs, current_dir, visited)
        visited[v] = nil
      else
        if is_string(v) and v ~= "" then
          local attr = lfs.attributes(current_dir .. v)
          if lfs.attributes(current_dir .. v) then
            if attr.mode == "directory" then
              dirs[#dirs + 1] = v
            else
              fs[#fs + 1] = current_dir .. v
            end
          end
        end
      end
    end
    return dirs, fs
  end

  go_through = function(t, dirs, fs, current_dir)
    return impl(t, dirs, fs, current_dir, { })
  end

  luarocks_list_rockspec_files = function(filename, current_dir)
    arguments(
        "string", filename,
        "string", current_dir
      )
    local env = luarocks_load_rockspec(filename)

    local files = { filename }
    local directories = { }

    -- filling files and directories table
    directories, files = go_through(env.build, directories, files, current_dir)

    -- scanning directories and adding all files found to files table
    for i = 1, #directories do
      local curr_dir_files, err = assert(find_all_files(current_dir .. directories[i], "."))
      for j = 1, #curr_dir_files  do
        files[#files + 1] = curr_dir_files[j]
      end
    end

    return files
  end
end

--------------------------------------------------------------------------------

return
{
  luarocks_read = luarocks_read;
  luarocks_show_rock_dir = luarocks_show_rock_dir;
  luarocks_exec = luarocks_exec;
  luarocks_exec_no_sudo = luarocks_exec_no_sudo;
  luarocks_exec_dir = luarocks_exec_dir;
  luarocks_admin_exec_dir = luarocks_admin_exec_dir;
  luarocks_remove_forced = luarocks_remove_forced;
  luarocks_ensure_rock_not_installed_forced = luarocks_ensure_rock_not_installed_forced;
  luarocks_make_in = luarocks_make_in;
  luarocks_exec_dir_no_sudo = luarocks_exec_dir_no_sudo;
  luarocks_pack_to = luarocks_pack_to;
  luarocks_admin_make_manifest = luarocks_admin_make_manifest;
  luarocks_load_manifest = luarocks_load_manifest;
  luarocks_get_rocknames_in_manifest = luarocks_get_rocknames_in_manifest;
  luarocks_install_from = luarocks_install_from;
  luarocks_parse_installed_rocks = luarocks_parse_installed_rocks;
  luarocks_load_rockspec = luarocks_load_rockspec;
  luarocks_list_rockspec_files = luarocks_list_rockspec_files;
}
