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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include <shutils.h>
#include <gpioutils.h>

void gpio_led_trig_set(int led, const char* trig_name)
{
	char led_path[128];
	const char *led_name = led_to_name(led);

	if (led_name && trig_name) {
		sprintf(led_path, "/sys/devices/platform/leds/leds/%s/trigger", led_name);
		fput_string(led_path, trig_name);
	}
}

void gpio_led_set(int led, int value)
{
	char led_path[128];
	const char *led_name = led_to_name(led);

	if (led_name) {
		sprintf(led_path, "/sys/devices/platform/leds/leds/%s/brightness", led_name);
		fput_int(led_path, value);
	}
}
