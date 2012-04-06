---------------------------------------------------------------------------------
-- cookie_jar.lua: web cookie archive
-- This file is a part of lua-aplicado library
-- Copyright (c) lua-nucleo authors (see file `COPYRIGHT` for the license)
---------------------------------------------------------------------------------

local setmetatable = setmetatable
local table_sort = table.sort
local os_date = os.date
local os_time = os.time

local getdate = require 'getdate'
local strptime = getdate.strptime

local socket_url = require 'socket.url'
local url_parse = socket_url.parse

local arguments,
      method_arguments
      = import 'lua-nucleo/args.lua'
      {
        'arguments',
        'method_arguments'
      }

local make_concatter = import 'lua-nucleo/string.lua' { 'make_concatter' }

---------------------------------------------------------------------------------

--
-- Web cookie archive (Jar)
--
-- refer to http://tools.ietf.org/html/rfc6265
--

local make_cookie_jar
do

  ------------------------------------------------------------------------------
  -- helper functions
  ------------------------------------------------------------------------------

  --
  -- parse date string into unix time
  --
  local parse_date_string_to_unix_time = function(date_string, time_fn)
    arguments(
        'string', date_string,
        'function', time_fn
      )
    -- ISO
    local t = strptime(date_string, '%Y-%m-%d %H:%M:%S GMT%z')
    -- RFC6265
    if not t then
      t = strptime(date_string, '%b %d %H:%M:%S %Y GMT%z')
    end
    if t then
      t = time_fn(t)
    end
    return t
  end

  --
  -- parse URL into components
  --
  local get_url_components = function(url)
    if not url then
      return
      {
        secure = false;
        domain = '';
        path = '/';
      }
    end
    arguments(
        'string', url
      )
    local uri = url_parse(url)
    -- let's not guess hostname
    if not uri.host then
      error('Invalid URL: "' .. url .. '"')
    end
    -- port is purely optional
    if uri.port then
      uri.host = uri.host .. ':' .. uri.port
    end
    return
    {
      secure = uri.scheme == 'https';
      domain = uri.host;
      -- no explicit path means root path
      path = uri.path or '/';
    }
  end

  --
  -- loose check on whether argument looks like an IPv4 address
  -- in dot-decimal notation
  --
  -- TODO: write strict version: https://redmine.iphonestudio.ru/issues/3551
  --
  local is_ipv4 = function(hostname)
    arguments(
        'string', hostname
      )
    -- N.B. ipv6 contains no dots, so ipv4 pattern be checked
    local parts = { hostname:match('^(%d+)%.(%d+)%.(%d+)%.(%d+)%$') }
    if #parts ~= 4 then
      return false
    end
    -- loose check on whether parts belong to [0..255]
    for i = 1, 4 do
      local x = tonumber(parts[i])
      if not x or x > 255 then
        return false
      end
    end
    return true
  end

  --
  -- check whether request_domain domain-matches cookie_domain
  --
  -- for details refer to  http://tools.ietf.org/html/rfc6265#section-5.1.3
  --
  local is_domain_matched = function(request_domain, cookie_domain)
    arguments(
        'string', request_domain,
        'string', cookie_domain
      )
    request_domain = request_domain:lower()
    cookie_domain = cookie_domain:lower()
    -- The domain string and the string are identical.
    if request_domain == cookie_domain then
      return true
    end
    -- The domain string is a suffix of the string.
    -- The last character of the string that is not included in the
    -- domain string is a %x2E (".") character.
    -- N.B. reject IP addresses
    if not is_ipv4(request_domain)
      and not is_ipv4(cookie_domain)
      and request_domain:match('^%w[%w_-]*%.' .. cookie_domain)
    then
      return true
    end
    return false
  end

  --
  -- check whether request_path path-matches cookie_path
  --
  -- for details refer to  http://tools.ietf.org/html/rfc6265#section-5.1.4
  --
  local is_path_matched = function(request_path, cookie_path)
    arguments(
        'string', request_path,
        'string', cookie_path
      )
    -- The cookie-path and the request-path are identical.
    if request_path == cookie_path then
      return true
    end
    -- The cookie-path is a prefix of the request-path, and the last
    -- character of the cookie-path is %x2F ("/").
    local start, finish = request_path:find(cookie_path, nil, true)
    if start == 1 and cookie_path:sub(#cookie_path) == '/' then
      return true
    end
    -- The cookie-path is a prefix of the request-path, and the first
    -- character of the request-path that is not included in the cookie-path
    -- is a %x2F ("/") character.
    if start == 1 and request_path:sub(finish + 1, finish + 1) == '/' then
      return true
    end
    return false
  end

  --
  -- sort function to help order cookies.
  -- by path, descending, then index, ascending
  --
  local cookie_less = function(lhs, rhs)
    -- cookies of the same path sorted by order of creation
    if lhs.path == rhs.path then
      return lhs.index < rhs.index
    end
    -- cookies of different paths sorted by path: longer path goes earlier
    return lhs.path > rhs.path
  end

  ------------------------------------------------------------------------------
  -- cookie archive methods
  ------------------------------------------------------------------------------

  -- get table of all valid cookies in archive
  -- N.B. we fix timestamp for all iterations
  local get_all = function(self)
    method_arguments(
        self
      )

    local timestamp = self.time_fn_()

    -- purge expired cookies
    -- N.B. http://tools.ietf.org/html/rfc6265 --
    --   The user agent MUST evict all expired cookies from the cookie store
    --   if, at any time, an expired cookie exists in the cookie store.
    local valid = { }
    for i = 1, #self.jar_ do
      local c = self.jar_[i]
      if not c.expires or c.expires > timestamp then
        valid[#valid + 1] = c
      end
    end
    self.jar_ = valid

    -- return storage
    return valid

  end

  -- restore archive original state
  local reset = function(self)

    self.jar_ = { }

  end

  -- put cookie into archive, overwriting existing one if
  -- name, domain and path cookie attributes all match
  local put = function(self, cookie)
    method_arguments(
        self,
        'table', cookie
      )

    -- if cookie with same name, domain and path exists,
    -- replace it
    local all = self:get_all()
    for i = 1, #all do
      local c = all[i]
      if c.name == cookie.name
        and c.domain == cookie.domain
        and c.path == cookie.path
      then
        cookie.old_value = c.old_value
        all[i] = cookie
        return
      end
    end

    -- insert new cookie
    self.jar_[#self.jar_ + 1] = cookie

  end

  -- get cookie by name, domain and path
  local get = function(self, name, domain, path)
    method_arguments(
        self,
        'string', name
      )

    -- find the cookie
    local all = self:get_all()
    for i = 1, #all do
      local c = all[i]
      if c.name == name
        and (not domain or c.domain == domain)
        and (not path or c.path == path)
      then
        return c
      end
    end

  end

  -- given Set-Cookie: HTTP header and URL, update the archive
  -- N.B. we fix timestamp for all iterations
  local update = function(self, header, url)
    method_arguments(
        self,
        'string', header
      )

    local timestamp = self.time_fn_()

    -- update old values
    local all = self:get_all()
    for i = 1, #all do
      local c = all[i]
      c.old_value = c.value
    end

    -- update values

    -- get request domain and path
    local uri = url and get_url_components(url)
    if uri and uri.path == '' then
      uri.path = '/'
    end

    -- compensate for ambiguous comma in Expires attribute
    -- N.B. without this commas in Expires can clash with
    -- commas delimiting chunks of composite header
    header = (header .. ','):gsub(
        '[Ee][Xx][Pp][Ii][Rr][Ee][Ss]%s*=%s*%w-,%s*(.-[;,])',
        'expires=%1'
      )

    -- for each comma separated chunk in header...
    for chunk in header:gmatch('%s*(.-)%s*,') do

      -- extract cookie name, value and optional trailer
      -- containing various attributes
      local name, value, attrs = (chunk .. ';'):match(
          '%s*(.-)%s*=%s*(.-)%s*;(.*)'
        )

      -- cookie
      local cookie =
      {
        name = name;
        value = value;
      }

      -- parse key/value attributes, if any
      if attrs then

        attrs = attrs:gsub(
            '%s*([%a_][%w_-]*)%s*=%s*(.-)%s*;',
            function(attr, value)
              attr = attr:lower()
              -- Expires attribute
              -- http://tools.ietf.org/html/rfc6265#section-5.2.1
              if attr == 'expires' then
                local expires = parse_date_string_to_unix_time(
                    value,
                    self.time_fn_
                  )
                cookie[attr] = expires
              -- Max-Age attribute
              -- http://tools.ietf.org/html/rfc6265#section-5.2.2
              elseif attr == 'max-age' then
                local delta = tonumber(value)
                if delta then
                  if delta > 0 then
                    cookie.expires = timestamp + delta
                  else
                    cookie.expires = 0
                  end
                end
              -- Domain attribute
              -- http://tools.ietf.org/html/rfc6265#section-5.2.3
              elseif attr == 'domain' then
                if value ~= '' then
                  -- drop leading dot
                  if value:sub(1, 1) == '.' then
                    value = value:sub(2)
                  end
                  cookie[attr] = value:lower()
                end
              -- Path attribute
              -- http://tools.ietf.org/html/rfc6265#section-5.2.4
              elseif attr == 'path' then
                cookie[attr] = value
              end
              -- consume attribute
              return ''
            end
          )

        -- parse flag attributes
        for attr in attrs:gmatch('%s*([%w_-]-)%s*;') do
          attr = attr:lower()
          -- HttpOnly and Secure attributes
          -- http://tools.ietf.org/html/rfc6265#section-5.2.5
          -- http://tools.ietf.org/html/rfc6265#section-5.2.6
          if attr == 'httponly' or attr == 'secure' then
            cookie[attr] = true
          end
        end

      end

      -- set default values for optional attributes
      if not cookie.domain then
        cookie.domain = uri and uri.domain
      end
      -- http://tools.ietf.org/html/rfc6265#section-5.1.4
      if not cookie.path then
        cookie.path = uri and uri.path:match('^(.*)/')
        if cookie.path == '' then
          cookie.path = '/'
        end
      end

      -- check attributes validity
      -- http://tools.ietf.org/html/rfc6265#section-5.3
      local valid = true

      if not cookie.name then
        valid = false
      end

      -- The value for the Domain attribute contains no embedded dots,
      -- and the value is not .local
      if cookie.domain then
        local dot = cookie.domain:find('.', 2, true)
        if not dot or dot == #cookie.domain then
          valid = false
        end
      end

      -- If the canonicalized request-host does not domain-match the
      -- domain-attribute.
      if cookie.domain
        and uri
        and not is_domain_matched(uri.domain, cookie.domain)
      then
        valid = false
      end

      -- update the cookie
      -- http://tools.ietf.org/html/rfc6265#section-5.3
      if valid then
        self:put(cookie)
      end

    end

  end

  -- get table of cookies matching provided URL.
  -- pass true as prune_httponly to not include httponly cookies
  local get_for_url = function(self, url, prune_httponly)

    -- get request domain and path
    local uri = get_url_components(url)

    -- collect relevant cookies
    -- http://tools.ietf.org/html/rfc6265#section-5.4
    local relevant = { }
    local all = self:get_all()
    for i = 1, #all do
      local cookie = all[i]
      if is_domain_matched(uri.domain, cookie.domain or '')
        and is_path_matched(uri.path, cookie.path or '/')
        -- don't send secure cookies via insecure path
        and (uri.secure or not cookie.secure)
        -- don't send httponly cookies if `http_only` specified
        and not (prune_httponly and cookie.httponly)
      then
        relevant[#relevant + 1] =
        {
          path = cookie.path;
          index = i;
          name = cookie.name;
          value = cookie.value;
        }
      end
    end

    -- sort cookies
    table_sort(relevant, cookie_less)

    return relevant

  end

  -- get string suitable to pass as Cookie: HTTP header for
  -- HTTP request to the specified URL
  local format_header_for_url = function(self, url, prune_httponly)

    local relevant = self:get_for_url(url, prune_httponly)

    -- map
    local cat, concat = make_concatter()
    for i = 1, #relevant do
      local cookie = relevant[i]
      cat(cookie.name .. '=' .. cookie.value)
    end

    -- glue
    return concat('; ')

  end

  --
  -- assertion helpers
  --

  local is_set = function(self, name, domain, path)
    method_arguments(
        self,
        'string', name
      )
    local c = self:get(name, domain, path)
    return not not (c and c.value and not c.old_value)
  end

  local is_updated = function(self, name, domain, path)
    method_arguments(
        self,
        'string', name
      )
    local c = self:get(name, domain, path)
    return not not (c and c.old_value and c.value ~= c.old_value)
  end

  local is_same = function(self, name, domain, path)
    method_arguments(
        self,
        'string', name
      )
    local c = self:get(name, domain, path)
    return not not (c and c.value == c.old_value)
  end

  ------------------------------------------------------------------------------
  -- expose cookie archive factory
  ------------------------------------------------------------------------------

  local jar_mt =
  {
    format_header_for_url = format_header_for_url;
    get = get;
    get_all = get_all;
    get_for_url = get_for_url;
    is_same = is_same;
    is_set = is_set;
    is_updated = is_updated;
    put = put;
    reset = reset;
    update = update;
  }

  -- since we deal with precise time expiry, we should be able
  -- to use custom time function
  make_cookie_jar = function(time_fn)
    time_fn = time_fn or os_time
    arguments(
        'function', time_fn
      )
    return setmetatable(
        {
          jar_ = { };
          time_fn_ = time_fn;
        },
        {
          __index = jar_mt;
        }
      )
  end

end

-- module
return
{
  make_cookie_jar = make_cookie_jar;
}
