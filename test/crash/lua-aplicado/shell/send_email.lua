--------------------------------------------------------------------------------
-- send_email.lua: dumb ad-hoc code to send emails
-- This file is a part of Lua-Aplicado library
-- Copyright (c) Lua-Aplicado authors (see file `COPYRIGHT` for the license)
--------------------------------------------------------------------------------

local assert
    = assert

local table_concat = table.concat

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

local fill_placeholders,
      make_concatter
      = import 'lua-nucleo/string.lua'
      {
        'fill_placeholders',
        'make_concatter'
      }

local shell_write
      = import 'lua-aplicado/shell.lua'
      {
        'shell_write'
      }

local is_table
      = import 'lua-nucleo/type.lua'
      {
        'is_table'
      }

--------------------------------------------------------------------------------

local send_email
local create_email

do
  local create_email_with_string_args = function(from, to, cc, bcc, subject, body)
    arguments(
        "string", from,
        "string", to,
        "string", cc,
        "string", bcc,
        "string", subject,
        "string", body
      )

    local cat, concat = make_concatter()

    cat "From: " (from) "\nTo: " (to) "\nCc: " (cc) "\nBcc: " (bcc) "\nSubject: " (subject)
      "\nContent-Type: text/plain; charset=utf-8\n\n" (body)

    return concat()
  end

  create_email = function(from, to, cc, bcc, subject, body)
    if is_table(cc) then
      cc = table_concat(cc, ", ")
    end
    if is_table(bcc) then
      bcc = table_concat(bcc, ", ")
    end
    arguments(
        "string", from,
        "string", to,
        "string", cc,
        "string", bcc,
        "string", subject,
        "string", body
      )

    return create_email_with_string_args(from, to, cc, bcc, subject, body)
  end

  send_email = function(from, to, cc, bcc, subject, body)
    arguments(
        "string", from,
        "string", to,
        "string", subject,
        "string", body
      )

    return shell_write(
        create_email(from, to, cc, bcc, subject, body),
        "sendmail",
        "-t"
      )
  end

end

--------------------------------------------------------------------------------

return
{
  send_email = send_email;
  create_email = create_email;
}
