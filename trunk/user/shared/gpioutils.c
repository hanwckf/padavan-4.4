#include <string.h>
#include <dirent.h>
#include "gpioutils.h"

const char* led_to_name(int led)
{
	switch (led)
	{
		case LED_PWR:
			return "power";
		case LED_WIFI:
			return "wifi";
		case LED_SW2G:
			return "sw2g";
		case LED_SW5G:
			return "sw5g";
		case LED_WAN:
			return "wan";
		case LED_LAN:
			return "lan";
		case LED_USB:
			return "usb";
		case LED_USB2:
			return "usb2";
		case LED_ROUTER:
			return "router";
		default:
			break;
	}
	return NULL;
}

int
search_gpio_led(void)
{
	static int leds = -1;
	const char* leds_dir = "/sys/devices/platform/leds/leds";
	DIR *dp;
	struct dirent *dirp;

	if (leds < 0)
	{
		leds = 0;
		if ((dp = opendir(leds_dir)) == NULL)
			return leds;

		while ((dirp = readdir(dp)) != NULL)
		{
			if (dirp->d_type == DT_DIR) {
				if (!strcmp(dirp->d_name, "power")) {
					leds |= LED_PWR;
				} else if (!strcmp(dirp->d_name, "wifi")) {
					leds |= LED_WIFI;
				} else if (!strcmp(dirp->d_name, "sw2g")) {
					leds |= LED_SW2G;
				} else if (!strcmp(dirp->d_name, "sw5g")) {
					leds |= LED_SW5G;
				} else if (!strcmp(dirp->d_name, "wan")) {
					leds |= LED_WAN;
				} else if (!strcmp(dirp->d_name, "lan")) {
					leds |= LED_LAN;
				} else if (!strcmp(dirp->d_name, "usb")) {
					leds |= LED_USB;
				} else if (!strcmp(dirp->d_name, "usb2")) {
					leds |= LED_USB2;
				} else if (!strcmp(dirp->d_name, "router")) {
					leds |= LED_ROUTER;
				}
			}
		}

		closedir(dp);
	}

	return leds;
}

int
search_gpio_btn(void)
{
	static int btns = -1;
	const char* dts_dir = "/sys/firmware/devicetree/base/gpio-keys-polled";
	DIR *dp;
	struct dirent *dirp;

	if (btns < 0)
	{
		btns = 0;
		if ((dp = opendir(dts_dir)) == NULL)
			return btns;
		
		while ((dirp = readdir(dp)) != NULL)
		{
			if (dirp->d_type == DT_DIR) {
				if (!strcmp(dirp->d_name, "reset")) {
					btns |= BTN_RESET;
				} else if (!strcmp(dirp->d_name, "wps")) {
					btns |= BTN_WPS;
				} else if (!strcmp(dirp->d_name, "fn1")) {
					btns |= BTN_FN1;
				} else if (!strcmp(dirp->d_name, "fn2")) {
					btns |= BTN_FN2;
				}
			}
		}

		closedir(dp);
	}

	return btns;
}
