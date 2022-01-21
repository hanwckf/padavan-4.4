------------------------------------------------
-- This file is part of the luci-app-ssr-plus update.lua
-- By Mattraks
------------------------------------------------
-- base64decoding
require 'nixio'
local b64decode = nixio.bin.b64decode
local function base64Decode(text)
	local raw = text
	if not text then return '' end
	text = text:gsub("%z", "")
	text = text:gsub("_", "/")
	text = text:gsub("-", "+")
	local mod4 = #text % 4
	text = text .. string.sub('====', mod4 + 1)
	local result = b64decode(text)
	
	if result then
		return result:gsub("%z", "")
	else
		return raw
	end
end

local function update()
	local gfwlist = io.open("/tmp/gfwlist_list_origin.conf", "r")
	local decode = gfwlist:read("*a")
	if not  decode:find("google") then
		decode = base64Decode(decode)
	end
		gfwlist:close()
		gfwlist = io.open("/tmp/gfwlist_list.conf", "w")
		gfwlist:write(decode)
		gfwlist:close()
end

update() 
