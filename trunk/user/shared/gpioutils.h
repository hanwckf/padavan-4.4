#ifndef __gpioutils_h_
#define __gpioutils_h_

extern const char* led_to_name(int led);
extern int search_gpio_led(void);
extern int search_gpio_btn(void);

#define BIT(x)			(1 << (x))

#define BTN_RESET		BIT(0)
#define BTN_WPS			BIT(1)
#define BTN_FN1			BIT(2)
#define BTN_FN2			BIT(3)

#define LED_PWR			BIT(0)
#define LED_WIFI		BIT(1)
#define LED_SW2G		BIT(2)
#define LED_SW5G		BIT(3)
#define LED_WAN			BIT(4)
#define LED_LAN			BIT(5)
#define LED_USB			BIT(6)
#define LED_USB2		BIT(7)
#define LED_ROUTER		BIT(8)

#endif
