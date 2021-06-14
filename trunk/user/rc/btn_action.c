#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rc.h"
#include "gpio_pins.h"
#include <gpioutils.h>

static void
ez_action_toggle_wifi2(void)
{
	if (get_enabled_radio_rt())
	{
		int i_radio_state = is_radio_on_rt();
		i_radio_state = !i_radio_state;
		
		notify_watchdog_wifi(0);
		
		logmessage("gpio_btn", "Perform ez-button toggle %s radio: %s", "2.4GHz", (i_radio_state) ? "ON" : "OFF");
		
		control_radio_rt(i_radio_state, 1);
	}
}

static void
ez_action_toggle_wifi5(void)
{
#if BOARD_HAS_5G_RADIO
	if (get_enabled_radio_wl())
	{
		int i_radio_state = is_radio_on_wl();
		i_radio_state = !i_radio_state;
		
		notify_watchdog_wifi(1);
		
		logmessage("gpio_btn", "Perform ez-button toggle %s radio: %s", "5GHz", (i_radio_state) ? "ON" : "OFF");
		
		control_radio_wl(i_radio_state, 1);
	}
#endif
}

static void
ez_action_change_wifi2(void)
{
	int i_radio_state;

	if (get_enabled_radio_rt())
	{
		i_radio_state = 0;
	}
	else
	{
		i_radio_state = 1;
		notify_watchdog_wifi(0);
	}

	nvram_wlan_set_int(0, "radio_x", i_radio_state);

	logmessage("gpio_btn", "Perform ez-button %s %s %s", (i_radio_state) ? "enable" : "disable", "2.4GHz", "radio");

#if defined(USE_RT3352_MII)
	mlme_radio_rt(i_radio_state);
#else
	restart_wifi_rt(i_radio_state, 0);
#endif
}

static void
ez_action_change_wifi5(void)
{
#if BOARD_HAS_5G_RADIO
	int i_radio_state;

	if (get_enabled_radio_wl())
	{
		i_radio_state = 0;
	}
	else
	{
		i_radio_state = 1;
		notify_watchdog_wifi(1);
	}

	nvram_wlan_set_int(1, "radio_x", i_radio_state);

	logmessage("gpio_btn", "Perform ez-button %s %s %s", (i_radio_state) ? "enable" : "disable", "5GHz", "radio");

	restart_wifi_wl(i_radio_state, 0);
#endif
}

static void
ez_action_change_guest_wifi2(void)
{
	int i_guest_state;

	if (get_enabled_guest_rt())
	{
		i_guest_state = 0;
	}
	else
	{
		i_guest_state = 1;
		notify_watchdog_wifi(0);
	}

	nvram_wlan_set_int(0, "guest_enable", i_guest_state);

	logmessage("gpio_btn", "Perform ez-button %s %s %s", (i_guest_state) ? "enable" : "disable", "2.4GHz", "AP Guest");

	control_guest_rt(i_guest_state, 1);
}

static void
ez_action_change_guest_wifi5(void)
{
#if BOARD_HAS_5G_RADIO
	int i_guest_state;

	if (get_enabled_guest_wl())
	{
		i_guest_state = 0;
	}
	else
	{
		i_guest_state = 1;
		notify_watchdog_wifi(1);
	}

	nvram_wlan_set_int(1, "guest_enable", i_guest_state);

	logmessage("gpio_btn", "Perform ez-button %s %s %s", (i_guest_state) ? "enable" : "disable", "5GHz", "AP Guest");

	control_guest_wl(i_guest_state, 1);
#endif
}

static void
ez_action_usb_saferemoval(int port)
{
#if defined (USE_USB_SUPPORT)
	char ez_name[24];

	strcpy(ez_name, "safe-removal USB");
#if (BOARD_NUM_USB_PORTS > 1)
	if (port == 1)
		strcat(ez_name, " #1");
	else if (port == 2)
		strcat(ez_name, " #2");
#else
	port = 0;
#endif
	logmessage("gpio_btn", "Perform ez-button %s...", ez_name);

	safe_remove_usb_device(port, NULL);
#endif
}

static void
ez_action_wan_down(void)
{
	if (get_ap_mode())
		return;

	logmessage("gpio_btn", "Perform ez-button %s...", "WAN disconnect");

	stop_wan();
}

static void
ez_action_wan_reconnect(void)
{
	if (get_ap_mode())
		return;

	logmessage("gpio_btn", "Perform ez-button %s...", "WAN reconnect");

	full_restart_wan();
}

static void
ez_action_wan_toggle(void)
{
	if (get_ap_mode())
		return;

	if (is_interface_up(get_man_ifname(0)))
	{
		logmessage("gpio_btn", "Perform ez-button %s...", "WAN disconnect");
		
		stop_wan();
	}
	else
	{
		logmessage("gpio_btn", "Perform ez-button %s...", "WAN reconnect");
		
		full_restart_wan();
	}
}

static void
ez_action_shutdown(void)
{
	logmessage("gpio_btn", "Perform ez-button %s...", "shutdown");

	sys_stop();
}

static void
ez_action_user_script(int script_param)
{
	const char *ez_script = "/etc/storage/ez_buttons_script.sh";

	if (!check_if_file_exist(ez_script))
		return;

	logmessage("gpio_btn", "Execute %s %d", ez_script, script_param);

	doSystem("%s %d", ez_script, script_param);
}

static void
ez_action_led_toggle(void)
{
	int is_show = (nvram_get_int("led_front_t")) ? 0 : 1;

	show_hide_front_leds(is_show);
}

void
btn_event_short(int btn_id)
{
	int ez_action = 0;
	int ez_param = 1;

	if (btn_id == BTN_FN1) {
		ez_action = nvram_get_int("fn1_action_short");
		ez_param = 3;
	} else if (btn_id == BTN_FN2) {
		ez_action = nvram_get_int("fn2_action_short");
		ez_param = 5;
	} else if (btn_id == BTN_WPS) {
		ez_action = nvram_get_int("ez_action_short");
	} else {
		return;
	}

	if (LED_PWR & search_gpio_led())
	{
		gpio_led_set(LED_PWR, get_state_led_pwr());
		if (ez_action != 10) {
			usleep(90000);
			LED_CONTROL(LED_PWR, LED_ON);
		}
	}

	switch (ez_action)
	{
	case 1: // WiFi radio ON/OFF trigger
		ez_action_toggle_wifi2();
		ez_action_toggle_wifi5();
		break;
	case 2: // WiFi 2.4GHz force Enable/Disable trigger
		ez_action_change_wifi2();
		break;
	case 3: // WiFi 5GHz force Enable/Disable trigger
		ez_action_change_wifi5();
		break;
	case 4: // WiFi 2.4 & 5GHz force Enable/Disable trigger
		ez_action_change_wifi2();
		ez_action_change_wifi5();
		break;
	case 5: // Safe removal all USB
		ez_action_usb_saferemoval(0);
		break;
	case 6: // WAN down
		ez_action_wan_down();
		break;
	case 7: // WAN reconnect
		ez_action_wan_reconnect();
		break;
	case 8: // WAN up/down toggle
		ez_action_wan_toggle();
		break;
	case 9: // Run user script
		ez_action_user_script(ez_param);
		break;
	case 10: // Front LED toggle
		ez_action_led_toggle();
		break;
	case 11: // WiFi AP Guest 2.4GHz Enable/Disable trigger
		ez_action_change_guest_wifi2();
		break;
	case 12: // WiFi AP Guest 5GHz Enable/Disable trigger
		ez_action_change_guest_wifi5();
		break;
	case 13: // WiFi AP Guest 2.4 & 5GHz Enable/Disable trigger
		ez_action_change_guest_wifi2();
		ez_action_change_guest_wifi5();
		break;
#if (BOARD_NUM_USB_PORTS > 1)
	case 21: // Safe removal USB #1
		ez_action_usb_saferemoval(1);
		break;
	case 22: // Safe removal USB #2
		ez_action_usb_saferemoval(2);
		break;
#endif
	}
}

void
btn_event_long(int btn_id)
{
	int ez_action = 0;
	int ez_param = 2;

	if (btn_id == BTN_FN1) {
		ez_action = nvram_get_int("fn1_action_long");
		ez_param = 4;
	} else if (btn_id == BTN_FN2) {
		ez_action = nvram_get_int("fn2_action_long");
		ez_param = 6;
	} else if (btn_id == BTN_WPS) {
		ez_action = nvram_get_int("ez_action_long");
	}

	if (LED_PWR & search_gpio_led())
	{
		if (ez_action == 7 || ez_action == 8)
			LED_CONTROL(LED_PWR, LED_OFF);
		else if (ez_action != 11)
			LED_CONTROL(LED_PWR, LED_ON);
	}

	switch (ez_action)
	{
	case 1: // WiFi 2.4GHz force Enable/Disable trigger
		ez_action_change_wifi2();
		break;
	case 2: // WiFi 5GHz force Enable/Disable trigger
		ez_action_change_wifi5();
		break;
	case 3: // WiFi 2.4 & 5GHz force Enable/Disable trigger
		ez_action_change_wifi2();
		ez_action_change_wifi5();
		break;
	case 4: // Safe removal all USB
		ez_action_usb_saferemoval(0);
		break;
	case 5: // WAN down
		ez_action_wan_down();
		break;
	case 6: // WAN reconnect
		ez_action_wan_reconnect();
		break;
	case 7: // Router reboot
		sys_exit();
		break;
	case 8: // Router shutdown
		ez_action_shutdown();
		break;
	case 9: // WAN up/down toggle
		ez_action_wan_toggle();
		break;
	case 10: // Run user script
		ez_action_user_script(ez_param);
		break;
	case 11: // Front LED toggle
		ez_action_led_toggle();
		break;
	case 12: // WiFi AP Guest 2.4GHz Enable/Disable trigger
		ez_action_change_guest_wifi2();
		break;
	case 13: // WiFi AP Guest 5GHz Enable/Disable trigger
		ez_action_change_guest_wifi5();
		break;
	case 14: // WiFi AP Guest 2.4 & 5GHz Enable/Disable trigger
		ez_action_change_guest_wifi2();
		ez_action_change_guest_wifi5();
		break;
	case 15: // Reset settings
		erase_nvram();
		erase_storage();
		sys_exit();
		break;
#if (BOARD_NUM_USB_PORTS > 1)
	case 21: // Safe removal USB #1
		ez_action_usb_saferemoval(1);
		break;
	case 22: // Safe removal USB #2
		ez_action_usb_saferemoval(2);
		break;
#endif
	}
}

void
btn_reset_action(void)
{
	erase_nvram();
	erase_storage();
	sys_exit();
}
