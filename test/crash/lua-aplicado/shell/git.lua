--------------------------------------------------------------------------------
-- git.lua: dumb ad-hoc code to work with git
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

local assert_is_table,
      assert_is_nil
      = import 'lua-nucleo/typeassert.lua'
      {
        'assert_is_table',
        'assert_is_nil'
      }

local split_by_char
      = import 'lua-nucleo/string.lua'
      {
        'split_by_char'
      }

local shell_exec,
      shell_read
      = import 'lua-aplicado/shell.lua'
      {
        'shell_exec',
        'shell_read'
      }

--------------------------------------------------------------------------------

local git_format_command = function(path, command, ...)
  return
    "cd", path,
    "&&", "git", command, ...
end

local git_exec = function(path, command, ...)
  return shell_exec(git_format_command(path, command, ...))
end

local git_read = function(path, command, ...)
  return shell_read(git_format_command(path, command, ...))
end

local git_get_tracking_branch_name_of_HEAD = function(path)
  -- Will work only on recent Git versions
  -- TODO: This spams into stderr too loud if branch has no remotes
  local full_name = git_read(path, "rev-parse", "--symbolic-full-name", "@{u}")
  return full_name:match("^refs/remotes/(.-)%s*$") -- May be nil.
end

local git_update_index = function(path)
  assert(git_exec(
      path, "update-index", "-q", "--refresh"
    ) == 0)

  -- TODO: HACK! Remove when Git is fixed.
  -- http://thread.gmane.org/gmane.comp.version-control.git/164216
  require('socket').sleep(0.5)
end

-- WARNING: needs git_update_index()!
local git_is_dirty = function(path)
  return git_exec(
      path, "diff-index", "--exit-code", "--quiet", "HEAD"
    ) ~= 0
end

-- WARNING: needs git_update_index()!
local git_has_untracked_files = function(path)
  return #git_read(
      path, "ls-files", "--exclude-standard", "--others"
    ) ~= 0 -- TODO: Hack
end

local git_are_branches_different = function(path, lhs_branch, rhs_branch)
  return #git_read(
      path,
      "rev-list", "--left-right", lhs_branch .. "..." .. rhs_branch
    ) ~= 0 -- TODO: Hack
end

local git_is_file_changed_between_revisions = function(
    path,
    filename,
    lhs_branch,
    rhs_branch
  )
  return #git_read(
      path,
      "rev-list", "--left-right", lhs_branch .. "..." .. rhs_branch, "--", filename
    ) ~= 0 -- TODO: Hack
end

-- TODO: Check that directory is inside git repo
local git_add_directory = function(path, dir)
  assert(git_exec(
      path, "add", dir .. "/"
    ) == 0)
end

local git_commit_with_editable_message = function(path, message)
  assert(git_exec(
      path, "commit", "--edit", "-m", message
    ) == 0)
end

local git_commit_with_message = function(path, message)
  assert(git_exec(
      path, "commit", "-m", message
    ) == 0)
end

local git_push_all = function(path)
  assert(git_exec(
      path, "push", "--all"
    ) == 0)
end

local git_is_directory_dirty = function(path, directory)
  return git_exec(
      path, "diff-index", "--exit-code", "--quiet", "HEAD", "--", directory
   ) ~= 0
end

-- Note that this function intentionally does not try to do any value coersion.
-- Every leaf value is a string.
local git_load_config = function(path)
  local config_str = git_read(path, "config", "--list")

  local result = { }

  for line in config_str:gmatch("(.-)\n") do
    local name, value = line:match("^(.-)=(.*)$")

    local val = result

    local keys = split_by_char(name, ".")
    for i = 1, #keys - 1 do
      local key = keys[i]

      assert_is_table(val)
      if val[key] == nil then
        local t = { }
        val[key] = t
      end
      val = val[key]
    end

    -- Note that the same value can be overridden several times in config
    -- This seems to be OK
    val[keys[#keys]] = value
  end

  return result
end

-- Returns false if remote not found
local git_config_get_remote_url = function(git_config, remote_name)
  arguments(
      "table", git_config,
      "string", remote_name
    )

  local remotes = git_config.remote
  if not remotes then
    return false
  end

  local remote = remotes[remote_name]
  if not remote then
    return false
  end

  return remote.url or false
end

local git_remote_rm = function(path, remote_name)
  assert(git_exec(
      path, "remote", "rm", remote_name
    ) == 0)
end

local git_remote_add = function(path, remote_name, url, fetch)
  if fetch then
    assert(git_exec(
        path, "remote", "add", "-f", remote_name, url
      ) == 0)
  else
    assert(git_exec(
        path, "remote", "add", remote_name, url
      ) == 0)
  end
end

local git_init_subtree = function(
    path,
    remote_name,
    url,
    branch,
    relative_path,
    commit_message,
    is_interactive
  )
  git_remote_add(path, remote_name, url, true) -- with fetch

  assert(git_exec(
      path, "merge", "-s", "ours", "--no-commit", remote_name .. "/" .. branch
    ) == 0)

  assert(git_exec(
      path, "read-tree", "--prefix="..relative_path,   "-u", remote_name .. "/" .. branch
    ) == 0)

  if is_interactive then
    git_commit_with_editable_message(path, commit_message)
  else
    git_commit_with_message(path, commit_message)
  end
end

local git_pull_subtree = function(path, remote_name, branch)
  assert(git_exec(
      path, "pull", "-s", "subtree", remote_name, branch
    ) == 0)
end

--------------------------------------------------------------------------------

return
{
  git_format_command = git_format_command;
  git_exec = git_exec;
  git_read = git_read;
  git_get_tracking_branch_name_of_HEAD = git_get_tracking_branch_name_of_HEAD;
  git_update_index = git_update_index;
  git_is_dirty = git_is_dirty;
  git_has_untracked_files = git_has_untracked_files;
  git_are_branches_different = git_are_branches_different;
  git_is_file_changed_between_revisions = git_is_file_changed_between_revisions;
  git_add_directory = git_add_directory;
  git_commit_with_editable_message = git_commit_with_editable_message;
  git_commit_with_message = git_commit_with_message;
  git_push_all = git_push_all;
  git_is_directory_dirty = git_is_directory_dirty;
  git_load_config = git_load_config;
  git_config_get_remote_url = git_config_get_remote_url;
  git_remote_rm = git_remote_rm;
  git_remote_add = git_remote_add;
  git_init_subtree = git_init_subtree;
  git_pull_subtree = git_pull_subtree;
}
