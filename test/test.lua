-- TODO: Scrap these hacks and write a proper test suite.

pcall(require, 'luarocks.require')

local luatexts = require 'luatexts'

assert(luatexts.load()) -- TODO: Remove

error("TODO: Write tests!")

print("OK")
