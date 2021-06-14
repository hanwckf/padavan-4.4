#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "rc.h"
#include "gpio_pins.h"
#include <gpioutils.h>

#ifdef GPIO_BTN_DBG
#define btn_dbg(fmt, args...) logmessage("gpio_btn", fmt, ## args)
#else
#define btn_dbg(fmt, args...) do {} while(0)
#endif

#define UEVENT_MSG_LEN	2048

#define BTN_PRESSED		1
#define BTN_RELEASED	2

#define RECV_PERIOD_US		100 * 1000	/* 100ms */
#define CANCEL_COUNT		8			/* 800ms */
#define LONG_COUNT		30			/* 3s */
#define RESET_COUNT		50			/* 5s */

static int running = 1;

typedef struct _btn_event
{
	char event;
	int btn_id;
} btn_event;

static struct btn_entrys_t
{
	const int btn_id;
	const char *btn_name;
	int count;
} btn_entrys[] = {
	{ BTN_RESET, "reset", 0 },
	{ BTN_WPS, "wps", 0 },
	{ BTN_FN1, "fn1", 0 },
	{ BTN_FN2, "fn2", 0 },
	{ 0, NULL, 0 },
};

int get_state_led_pwr(void)
{
	int i_led;

	if (nvram_get_int("front_led_pwr") == 0) {
		// POWER always OFF
		i_led = LED_ON;
	} else {
		// POWER always ON
		i_led = LED_OFF;
	}

	return i_led;
}

static int btn_name_to_id(const char* name)
{
	const struct btn_entrys_t *btn;

	if (name) {
		for (btn = &btn_entrys[0]; btn->btn_id; btn++)
			if (!strcmp(name, btn->btn_name))
				return btn->btn_id;
	}

	return 0;
}

static int btn_uevent_init(void)
{
	struct sockaddr_nl nls;
	struct timeval tv;
	int nlbufsize = 512 * 1024;
	int fd;

	tv.tv_sec = 0;
	tv.tv_usec = RECV_PERIOD_US;

	memset(&nls, 0, sizeof(struct sockaddr_nl));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;

	if ((fd = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT)) == -1) {
		btn_dbg("Failed to open hotplug socket: %s\n", strerror(errno));
		exit(1);
	}
	if (bind(fd, (void *)&nls, sizeof(struct sockaddr_nl))) {
		btn_dbg("Failed to bind hotplug socket: %s\n", strerror(errno));
		exit(1);
	}

	setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &nlbufsize, sizeof(nlbufsize));
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	return fd;
}

static void nlmsg_handle(const char *msg, int len, btn_event *ev)
{
	int i = 0;
	int btn_id = 0;
	char btn_event = 0;

	while (i < len)
	{
		int l = strlen(msg+i) + 1;
		char *e = strstr(&msg[i], "=");

		if (e) {
			*e = '\0';
			//btn_dbg("nlmsg: %s = %s\n", &msg[i], &e[1]);
			if (!strcmp(&msg[i], "ACTION")) {
				if (!strcmp(&e[1], "pressed")) {
					btn_event = BTN_PRESSED;
				} else if (!strcmp(&e[1], "released")) {
					btn_event = BTN_RELEASED;
				} else {
					return;
				}
			}
			if (btn_event && !strcmp(&msg[i], "BUTTON")) {
				btn_id = btn_name_to_id(&e[1]);
				if (btn_id) {
					ev->event = btn_event;
					ev->btn_id = btn_id;
				}
				return;
			}
		}
		i += l;
	}
}

static void sig_handle(int signum)
{
	running = 0;
}

static void btn_released_handle(int btn_id, int count)
{
	btn_dbg("released_handle: btn=0x%x, count=%d\n", btn_id, count);

	if (btn_id == BTN_RESET) {
		if (count > RESET_COUNT) {
			btn_dbg("perform RESET!!\n");
			LED_CONTROL(LED_PWR, LED_OFF);
			btn_reset_action();
		} else if (count > 0) {
			btn_dbg("RESET pressed time too short!\n");
			LED_CONTROL(LED_PWR, LED_ON);
		}
	} else {
		if (count > LONG_COUNT) {
			btn_dbg("perform long event, btn=0x%x\n", btn_id);
			btn_event_long(btn_id);
		} else if (count > 0 && count < CANCEL_COUNT) {
			btn_dbg("perform short event, btn=0x%x\n", btn_id);
			btn_event_short(btn_id);
		}
	}
}

static void btn_ev_handle(btn_event *ev)
{
	struct btn_entrys_t *btn;
	int cnt;
	int pwr_led;

	for (btn = &btn_entrys[0]; btn->btn_id; btn++)
	{
		if (ev->btn_id == btn->btn_id)
		{
			if (ev->event == BTN_PRESSED) {
				(btn->count)++;
				if (LED_PWR & search_gpio_led()) {
					pwr_led = get_state_led_pwr();

					if (btn->btn_id == BTN_RESET && btn->count == 1) {
						gpio_led_set(LED_PWR, pwr_led);
					} else if ((btn->btn_id == BTN_RESET && btn->count > RESET_COUNT) ||
						(btn->btn_id != BTN_RESET && btn->count > LONG_COUNT)) {
						btn_dbg("you can release btn now!\n");
						gpio_led_set(LED_PWR, (btn->count % 2) ? !pwr_led : pwr_led);
					}
				}
			} else if (ev->event == BTN_RELEASED) {
				cnt = btn->count;
				btn->count = 0;
				ev->event = 0;
				ev->btn_id = 0;
				btn_released_handle(btn->btn_id, cnt);
			}
			break;
		}
	}
}

int start_gpio_btn(void)
{
	if (pids("gpio_btn"))
		return 0;

	return eval("/sbin/gpio_btn");
}

int btn_main(int argc, char *argv[])
{
	char buf[UEVENT_MSG_LEN] = {0};
	int msglen = 0;
	int fd;
	btn_event btn_ev;
	struct sigaction sa;
	pid_t pid;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handle;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	if (daemon(0, 0) < 0) {
		perror("daemon");
		exit(errno);
	}

	pid = getpid();

	/* never invoke oom killer */
	oom_score_adjust(pid, OOM_SCORE_ADJ_MIN);

	fd = btn_uevent_init();

	memset(&btn_ev, 0, sizeof(btn_ev));

	logmessage("gpio_btn", "service started.");

	while (running)
	{
		msglen = recv(fd, buf, UEVENT_MSG_LEN, 0);

		if (msglen > 0)
			nlmsg_handle(buf, msglen, &btn_ev);

		if (btn_ev.btn_id && btn_ev.event) {
			//btn_dbg("btn_ev: btn=0x%x, event=%d\n", btn_ev.btn_id, btn_ev.event);
			btn_ev_handle(&btn_ev);
		}
	}

	close(fd);

	return 0;
}
