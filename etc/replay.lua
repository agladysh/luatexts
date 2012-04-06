pcall(require, 'luarocks.require')
require 'lua-nucleo.module'
require 'lua-nucleo.strict'
require = import 'lua-nucleo/require_and_declare.lua' { 'require_and_declare' }

math.randomseed(12345)

local ensure,
      ensure_equals,
      ensure_tequals,
      ensure_tdeepequals,
      ensure_strequals,
      ensure_error,
      ensure_error_with_substring,
      ensure_fails_with_substring,
      ensure_returns
      = import 'lua-nucleo/ensure.lua'
      {
        'ensure',
        'ensure_equals',
        'ensure_tequals',
        'ensure_tdeepequals',
        'ensure_strequals',
        'ensure_error',
        'ensure_error_with_substring',
        'ensure_fails_with_substring',
        'ensure_returns'
      }

local split_by_char = import 'lua-nucleo/string.lua' { 'split_by_char' }

local tset = import 'lua-nucleo/table-utils.lua' { 'tset' }

local tpretty = import 'lua-nucleo/tpretty.lua' { 'tpretty' }

local tstr = import 'lua-nucleo/tstr.lua' { 'tstr' }

local tserialize = import 'lua-nucleo/tserialize.lua' { 'tserialize' }

local tdeepequals = import 'lua-nucleo/tdeepequals.lua' { 'tdeepequals' }

local find_all_files,
      read_file
      = import 'lua-aplicado/filesystem.lua'
      {
        'find_all_files',
        'read_file'
      }

--------------------------------------------------------------------------------

-- TODO: ?! UGH! #3836
declare 'inf'
inf = 1/0

--------------------------------------------------------------------------------

local PREFIX = select(1, ...) or "tmp"
local OFFSET = tonumber(select(2, ...) or 1) or 1
local MODE = (select(3, ...) or "c"):lower()

local luatexts
if MODE == "c" then
  luatexts = require 'luatexts'
elseif MODE == "lua" then
  luatexts = require 'luatexts.lua'
else
  error("unknown mode")
end

io.stderr:write(
    "replay.lua:",
    " PREFIX: ", PREFIX,
    " OFFSET: ", OFFSET,
    " MODE: ", MODE,
    "\n"
  )
io.stderr:flush()

--------------------------------------------------------------------------------

local filenames = find_all_files(PREFIX, ".*%d+%.luatexts$")
table.sort(filenames)

local n_str

for i = 1, #filenames do
  local filename = filenames[i]
  n_str = assert(filename:match("^"..PREFIX.."/(%d+).luatexts$"))

  local n = assert(tonumber(n_str, 10))
  if n >= OFFSET then
    local tuple, tuple_size = assert(dofile(PREFIX.."/"..n_str..".lua"))
    local data = assert(read_file(filename))

    ensure_returns(
        "load " .. n_str,
        tuple_size + 1, { true, unpack(tuple, 1, tuple_size) },
        luatexts.load(data)
      )
  end
end

io.stdout:flush()
print("OK")
