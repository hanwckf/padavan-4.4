local cjson = require "cjson"
local server_section = arg[1]
local proto = arg[2]
local local_port = arg[3]

local ssrindext = io.popen("dbus get ssconf_basic_json_" .. server_section)
local servertmp = ssrindext:read("*all")
local server = cjson.decode(servertmp)

local trojan = {
	log_level = 99,
	run_type = proto,
	local_addr = "0.0.0.0",
	local_port = tonumber(local_port),
	remote_addr = server.server,
	remote_port = tonumber(server.server_port),
	udp_timeout = 60,
	-- 传入连接
	password = {server.password},
	-- 传出连接
	ssl = {
		verify = (server.insecure == "0") and true or false,
		verify_hostname = (server.tls == "1") and true or false,
		cert = "",
		cipher = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305",
		cipher_tls13 = "TLS_CHACHA20_POLY1305_SHA256",
		sni = server.tls_host,
		alpn = {"h2", "http/1.1"},
		curve = "",
		reuse_session = true,
		session_ticket = false,
		},
		tcp = {
			no_delay = true,
			keep_alive = true,
			reuse_port = true,
			fast_open = (server.fast_open == "1") and true or false,
			fast_open_qlen = 20
		}
}
print(cjson.encode(trojan))
