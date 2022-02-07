/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>

#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <sys/wait.h>

#include "rc.h"

#define WD_NORMAL_PERIOD	10		/* 10s */

#define WD_NOTIFY_ID_WIFI2	1
#define WD_NOTIFY_ID_WIFI5	2

#define WD_PID_FILE		"/var/run/watchdog.pid"

enum
{
	RADIO5_ACTIVE = 0,
	GUEST5_ACTIVE,
	RADIO2_ACTIVE,
	GUEST2_ACTIVE,
	ACTIVEITEMS
};

static int svcStatus[ACTIVEITEMS] = {-1, -1, -1, -1};

static int ntpc_timer = -1;
static int ntpc_server_idx = 0;
static int ntpc_tries = 0;

static int httpd_missing = 0;
static int dnsmasq_missing = 0;

static struct itimerval wd_itv;

static void
wd_alarmtimer(unsigned long sec, unsigned long usec)
{
	wd_itv.it_value.tv_sec  = sec;
	wd_itv.it_value.tv_usec = usec;
	wd_itv.it_interval = wd_itv.it_value;
	setitimer(ITIMER_REAL, &wd_itv, NULL);
}

#ifdef HTTPD_CHECK
#define DETECT_HTTPD_FILE "/tmp/httpd_check_result"
static int
httpd_check_v2()
{
	FILE *fp = NULL;
	int i, httpd_live, http_port;
	char line[80], *login_timestamp;
	long now;
	static int check_count_down = 3;
	static int httpd_timer = 0;

	/* skip 30 seconds after start watchdog */
	if (check_count_down)
	{
		check_count_down--;
		return 1;
	}

	/* check every 30 seconds */
	httpd_timer = (httpd_timer + 1) % 3;
	if (httpd_timer)
		return 1;

	/* check last http login */
	login_timestamp = nvram_safe_get("login_timestamp");
	if (strlen(login_timestamp) < 1)
		return 1;

	now = uptime();
	if (((unsigned long)(now - strtoul(login_timestamp, NULL, 10)) < 60))
		return 1;

#if defined (SUPPORT_HTTPS)
	/* check HTTPS only */
	if (nvram_get_int("http_proto") == 1)
		return 1;
#endif
	remove(DETECT_HTTPD_FILE);

	http_port = nvram_get_int("http_lanport");

	/* httpd will not count 127.0.0.1 */
	doSystem("wget -q http://127.0.0.1:%d/httpd_check.htm -O %s &", http_port, DETECT_HTTPD_FILE);

	httpd_live = 0;
	for (i=0; i < 3; i++)
	{
		if ((fp = fopen(DETECT_HTTPD_FILE, "r")) != NULL)
		{
			if ( fgets(line, sizeof(line), fp) != NULL )
			{
				if (strstr(line, "ASUSTeK"))
				{
					httpd_live = 1;
				}
			}
			
			fclose(fp);
		}
		
		if (httpd_live)
			break;
		
		/* check port changed */
		if (nvram_get_int("http_lanport") != http_port)
		{
			if (pids("wget"))
				system("killall wget");
			return 1;
		}
		
		sleep(1);
	}

	if (!httpd_live)
	{
		if (pids("wget"))
			system("killall wget");
		
		dbg("httpd is so dead!!!\n");
		
		return 0;
	}

	return 1;
}
#endif

static void
refresh_ntp(void)
{
	char *svcs[] = { "ntpd", NULL };
	char *ntp_addr[2], *ntp_server;

	kill_services(svcs, 3, 1);

	ntp_addr[0] = nvram_safe_get("ntp_server0");
	ntp_addr[1] = nvram_safe_get("ntp_server1");

	if (strlen(ntp_addr[0]) < 3)
		ntp_addr[0] = ntp_addr[1];
	else if (strlen(ntp_addr[1]) < 3)
		ntp_addr[1] = ntp_addr[0];

	if (strlen(ntp_addr[0]) < 3) {
		ntp_addr[0] = "pool.ntp.org";
		ntp_addr[1] = ntp_addr[0];
	}

	ntp_server = (ntpc_server_idx) ? ntp_addr[1] : ntp_addr[0];
	ntpc_server_idx = (ntpc_server_idx + 1) % 2;

	eval("/usr/sbin/ntpd", "-qt", "-S", NTPC_DONE_SCRIPT, "-p", ntp_server);

	logmessage("NTP Client", "Synchronizing time to %s.", ntp_server);
}

int
is_ntpc_updated(void)
{
	return (nvram_get_int("ntpc_counter") > 0) ? 1 : 0;
}

static void
ntpc_handler(void)
{
	int ntp_period = nvram_get_int("ntp_period");

	if (ntp_period < 1)
		return;

	if (ntp_period > 336)
		ntp_period = 336; // max two weeks

	ntp_period = ntp_period * 360;

	// update ntp every period time
	ntpc_timer = (ntpc_timer + 1) % ntp_period;
	if (ntpc_timer == 0) {
		setenv_tz();
		refresh_ntp();
	} else if (!is_ntpc_updated()) {
		int ntp_skip = 3;	// update every 30s
		
		ntpc_tries++;
		if (ntpc_tries > 60)
			ntp_skip = 30;	// update every 5m
		else if (ntpc_tries > 9)
			ntp_skip = 6;	// update every 60s
		
		if (!(ntpc_tries % ntp_skip))
			refresh_ntp();
	}
}

static void
inet_handler(int is_ap_mode)
{
	if (!is_ap_mode)
	{
		long i_deferred_wanup = nvram_get_int("deferred_wanup_t");
		if (i_deferred_wanup > 0 && uptime() >= i_deferred_wanup)
		{
			notify_rc("deferred_wan_connect");
			
			return;
		}
		
		if (has_wan_ip4(0) && has_wan_gw4())
		{
			/* sync time to ntp server if necessary */
			ntpc_handler();
		}
	}
	else
	{
		if (has_lan_ip4() && has_lan_gw4())
			ntpc_handler();
	}
}

int timecheck_reboot(char *activeSchedule)
{
	int active, current_time, current_date, Time2Active, Date2Active;
	time_t now;
	struct tm *tm;
	int i;

	setenv("TZ", nvram_safe_get("time_zone_x"), 1);

	time(&now);
	tm = localtime(&now);
	current_time = tm->tm_hour * 60 + tm->tm_min;
	current_date = 1 << (6-tm->tm_wday);
	active = 0;
	Time2Active = 0;
	Date2Active = 0;

	Time2Active = ((activeSchedule[7]-'0')*10 + (activeSchedule[8]-'0'))*60 + ((activeSchedule[9]-'0')*10 + (activeSchedule[10]-'0'));

	for(i=0;i<=6;i++) {
		Date2Active += (activeSchedule[i]-'0') << (6-i);
	}

	if ((current_time == Time2Active) && (Date2Active & current_date))	active = 1;

	//dbG("[watchdog] current_time=%d, ActiveTime=%d, current_date=%d, ActiveDate=%d, active=%d\n",
	//	current_time, Time2Active, current_date, Date2Active, active);

	return active;
}

/* Check for time-dependent service */
static int 
svc_timecheck(void)
{
	int activeNow;

#if BOARD_HAS_5G_RADIO
	if (get_enabled_radio_wl())
	{
		/* Initialize */
		if (nvram_match("reload_svc_wl", "1"))
		{
			nvram_set_int_temp("reload_svc_wl", 0);
			svcStatus[RADIO5_ACTIVE] = -1;
			svcStatus[GUEST5_ACTIVE] = -1;
		}
		
		activeNow = is_radio_allowed_wl();
		if (activeNow != svcStatus[RADIO5_ACTIVE])
		{
			svcStatus[RADIO5_ACTIVE] = activeNow;
			
			if (activeNow)
				notify_rc("control_wifi_radio_wl_on");
			else
				notify_rc("control_wifi_radio_wl_off");
		}
		
		if (svcStatus[RADIO5_ACTIVE] > 0)
		{
			activeNow = is_guest_allowed_wl();
			if (activeNow != svcStatus[GUEST5_ACTIVE])
			{
				svcStatus[GUEST5_ACTIVE] = activeNow;
				
				if (activeNow)
					notify_rc("control_wifi_guest_wl_on");
				else
					notify_rc("control_wifi_guest_wl_off");
			}
		}
		else
		{
			if (svcStatus[GUEST5_ACTIVE] >= 0)
				svcStatus[GUEST5_ACTIVE] = -1;
		}
	}
#endif

	if (get_enabled_radio_rt())
	{
		/* Initialize */
		if (nvram_match("reload_svc_rt", "1"))
		{
			nvram_set_int_temp("reload_svc_rt", 0);
			svcStatus[RADIO2_ACTIVE] = -1;
			svcStatus[GUEST2_ACTIVE] = -1;
		}
		
		activeNow = is_radio_allowed_rt();
		if (activeNow != svcStatus[RADIO2_ACTIVE])
		{
			svcStatus[RADIO2_ACTIVE] = activeNow;
			
			if (activeNow)
				notify_rc("control_wifi_radio_rt_on");
			else
				notify_rc("control_wifi_radio_rt_off");
		}
		
		if (svcStatus[RADIO2_ACTIVE] > 0)
		{
			activeNow = is_guest_allowed_rt();
			if (activeNow != svcStatus[GUEST2_ACTIVE])
			{
				svcStatus[GUEST2_ACTIVE] = activeNow;
				
				if (activeNow)
					notify_rc("control_wifi_guest_rt_on");
				else
					notify_rc("control_wifi_guest_rt_off");
			}
		}
		else
		{
			if (svcStatus[GUEST2_ACTIVE] >= 0)
				svcStatus[GUEST2_ACTIVE] = -1;
		}
	}
	
	char reboot_schedule[PATH_MAX];
	if (nvram_match("reboot_schedule_enable", "1"))
	{
		if (nvram_match("ntp_ready", "1"))
		{
			snprintf(reboot_schedule, sizeof(reboot_schedule), "%s", nvram_safe_get("reboot_schedule"));
			if (strlen(reboot_schedule) == 11 && atoi(reboot_schedule) > 2359)
			{
				if (timecheck_reboot(reboot_schedule))
				{
					logmessage("reboot scheduler", "[%s] The system is going down for reboot\n", __FUNCTION__);
	                sys_exit();
				}
			}
		}
		//else
		//	logmessage("reboot scheduler", "[%s] NTP sync error\n", __FUNCTION__);
	}

	char ss_schedule[PATH_MAX];
	if (nvram_match("ss_schedule_enable", "1"))
	{
		if (nvram_match("ntp_ready", "1"))
		{
			snprintf(ss_schedule, sizeof(ss_schedule), "%s", nvram_safe_get("ss_schedule"));
			if (strlen(ss_schedule) == 11 && atoi(ss_schedule) > 2359)
			{
				if (timecheck_reboot(ss_schedule))
				{
					//logmessage("reboot scheduler", "[%s] The system is going down for reboot\n", __FUNCTION__);
		            doSystem("/usr/bin/update_dlink.sh %s", "update");
					sleep(70);
				}
			}
		}
	}
	return 0;
}

static void
update_svc_status_wifi2()
{
	nvram_set_int_temp("reload_svc_rt", 0);
	svcStatus[RADIO2_ACTIVE] = is_radio_allowed_rt();

	if (svcStatus[RADIO2_ACTIVE] > 0)
		svcStatus[GUEST2_ACTIVE] = is_guest_allowed_rt();
	else
		svcStatus[GUEST2_ACTIVE] = -1;
}

static void
update_svc_status_wifi5()
{
#if BOARD_HAS_5G_RADIO
	nvram_set_int_temp("reload_svc_wl", 0);
	svcStatus[RADIO5_ACTIVE] = is_radio_allowed_wl();

	if (svcStatus[RADIO5_ACTIVE] > 0)
		svcStatus[GUEST5_ACTIVE] = is_guest_allowed_wl();
	else
		svcStatus[GUEST5_ACTIVE] = -1;
#endif
}

/* Sometimes, httpd becomes inaccessible, try to re-run it */
static void httpd_process_check(void)
{
	int httpd_is_run = pids("httpd");

	if (!httpd_is_run)
		httpd_missing++;
	else
		httpd_missing = 0;

	if (httpd_missing == 1)
		return;

	if ((!httpd_is_run
#ifdef HTTPD_CHECK
	    || !httpd_check_v2()
#endif
	    ) && nvram_match("httpd_started", "1"))
	{
		printf("## restart httpd ##\n");
		httpd_missing = 0;
		stop_httpd();
#ifdef HTTPD_CHECK
		system("killall -9 httpd");
		sleep(1);
		remove(DETECT_HTTPD_FILE);
#endif
		start_httpd(0);
	}
}

/* Sometimes, dnsmasq crashed, try to re-run it */
static void
dnsmasq_process_check(void)
{
	if (!is_dns_dhcpd_run())
		dnsmasq_missing++;
	else
		dnsmasq_missing = 0;
	
	if (dnsmasq_missing > 1) {
		dnsmasq_missing = 0;
		logmessage("watchdog", "dnsmasq is missing, start again!");
		start_dns_dhcpd(0);
	}
}

int
ntpc_updated_main(int argc, char *argv[])
{
	char *offset;
	int ntpc_counter;

	if (argc < 2)
		return -1;

	if (strcmp(argv[1], "step") != 0)
		return 0;

	ntpc_counter = nvram_get_int("ntpc_counter");
	nvram_set_int_temp("ntpc_counter", ntpc_counter + 1);

	offset = getenv("offset");
	if (offset) {
#if defined (USE_RTC_HCTOSYS)
		/* update current system time to RTC chip */
		system("hwclock -w");
#endif
		logmessage("NTP Client", "System time changed, offset: %ss", offset);
		sleep(5);
		nvram_set_int("ntp_ready", 1);
	}

	return 0;
}

static void
watchdog_on_sighup(void)
{
	setenv_tz();

	if (!is_ntpc_updated()) {
		ntpc_tries = 0;
		ntpc_timer = -1; // want call now
	}
}

static void
watchdog_on_sigusr1(void)
{
	int wd_notify_id = nvram_get_int("wd_notify_id");
	if (wd_notify_id == WD_NOTIFY_ID_WIFI2) {
		update_svc_status_wifi2();
	} else if (wd_notify_id == WD_NOTIFY_ID_WIFI5) {
		update_svc_status_wifi5();
	}
}

static void
watchdog_on_timer(void)
{
	int is_ap_mode = get_ap_mode();

	/* check for time-dependent services */
	svc_timecheck();

	/* http server check */
	httpd_process_check();

	/* DNS/DHCP server check */
	if (!is_ap_mode)
		dnsmasq_process_check();

	inet_handler(is_ap_mode);

	/* update kernel timezone daylight */
	setkernel_tz();

	storage_save_time(10);
}

static void
catch_sig_watchdog(int sig)
{
	switch (sig)
	{
	case SIGALRM:
		watchdog_on_timer();
		break;
	case SIGHUP:
		watchdog_on_sighup();
		break;
	case SIGUSR1:
		watchdog_on_sigusr1();
		break;
	case SIGTERM:
		remove(WD_PID_FILE);
		wd_alarmtimer(0, 0);
		exit(0);
		break;
	}
}

int
start_watchdog(void)
{
	if (pids("watchdog"))
		return 0;

	return eval("/sbin/watchdog");
}

void
notify_watchdog_time(void)
{
	if (pids("watchdog"))
		kill_pidfile_s(WD_PID_FILE, SIGHUP);
	else
		eval("/sbin/watchdog");
}

void
notify_watchdog_wifi(int is_5ghz)
{
	int wd_notify_id = (is_5ghz) ? WD_NOTIFY_ID_WIFI5 : WD_NOTIFY_ID_WIFI2;

	nvram_set_int_temp("wd_notify_id", wd_notify_id);
	kill_pidfile_s(WD_PID_FILE, SIGUSR1);
}

int
watchdog_main(int argc, char *argv[])
{
	FILE *fp;
	pid_t pid;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = catch_sig_watchdog;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaddset(&sa.sa_mask, SIGHUP);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);

	if (daemon(0, 0) < 0) {
		perror("daemon");
		exit(errno);
	}

	pid = getpid();

	/* never invoke oom killer */
	oom_score_adjust(pid, OOM_SCORE_ADJ_MIN);

	/* write pid */
	if ((fp = fopen(WD_PID_FILE, "w")) != NULL) {
		fprintf(fp, "%d", pid);
		fclose(fp);
	}

	nvram_set_int_temp("wd_notify_id", 0);

	/* set timer */
	wd_alarmtimer(WD_NORMAL_PERIOD, 0);

	/* most of time it goes to sleep */
	while (1) {
		pause();
	}

	return 0;
}

