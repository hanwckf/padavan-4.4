------------------------------------------------
-- This file is part of the luci-app-ssr-plus update.lua
-- By Mattraks
------------------------------------------------
-- match comments/title/whitelist/ip address/excluded_domain
local comment_pattern = "^[!\\[@]+"
local ip_pattern = "^%d+%.%d+%.%d+%.%d+"
local domain_pattern = "([%w%-%_]+%.[%w%.%-%_]+)[%/%*]*"
local excluded_domain = {"apple.com", "sina.cn", "sina.com.cn", "baidu.com", "byr.cn", "jlike.com", "weibo.com", "zhongsou.com", "youdao.com", "sogou.com", "so.com", "soso.com", "aliyun.com", "taobao.com", "jd.com", "qq.com"}
-- gfwlist parameter
local mydnsip = '127.0.0.1'
local mydnsport = '5353'
local ipsetname = 'gfwlist'


-- check excluded domain
local function check_excluded_domain(value)
	for k, v in ipairs(excluded_domain) do
		if value:find(v) then
			return true
		end
	end
end


local function generate_gfwlist()
	local domains = {}
	local out = io.open("/tmp/dnsmasq.dom/gfwlist_list.conf", "w")
	for line in io.lines("/etc/storage/gfwlist/gfwlist_listnew.conf") do
		if not (string.find(line, comment_pattern) or string.find(line, ip_pattern) or check_excluded_domain(line)) then
			local start, finish, match = string.find(line, domain_pattern)
			if (start) then
				domains[match] = true
			end
		end
	end
	for k, v in pairs(domains) do
		out:write(string.format("server=/%s/%s#%s\n", k, mydnsip, mydnsport))
		out:write(string.format("ipset=/%s/%s\n", k, ipsetname))
	end
	out:close()
end

generate_gfwlist()
	
