/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <math.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

#define SW0_NODE DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define PORT0		DT_GPIO_LABEL(SW0_NODE, gpios)
#define PIN0		DT_GPIO_PIN(SW0_NODE, gpios)
#define PIN0_FLAGS	DT_GPIO_FLAGS(SW0_NODE, gpios)
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define PORT0		""
#define PIN0		0
#define PIN0_FLAGS	0
#endif

#define SW1_NODE DT_ALIAS(sw1)

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
#define PORT1		DT_GPIO_LABEL(SW1_NODE, gpios)
#define PIN1		DT_GPIO_PIN(SW1_NODE, gpios)
#define PIN1_FLAGS	DT_GPIO_FLAGS(SW1_NODE, gpios)
#endif

#define SW2_NODE DT_ALIAS(sw2)

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
#define PORT2		DT_GPIO_LABEL(SW2_NODE, gpios)
#define PIN2		DT_GPIO_PIN(SW2_NODE, gpios)
#define PIN2_FLAGS	DT_GPIO_FLAGS(SW2_NODE, gpios)
#endif

#define SW3_NODE DT_ALIAS(sw3)

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
#define PORT3		DT_GPIO_LABEL(SW3_NODE, gpios)
#define PIN3		DT_GPIO_PIN(SW3_NODE, gpios)
#define PIN3_FLAGS	DT_GPIO_FLAGS(SW3_NODE, gpios)
#endif

#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED_PORT	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED		DT_GPIO_PIN(LED0_NODE, gpios)
#define LED_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED_PORT	""
#define LED		0
#define LED_FLAGS	0
#endif

/* Example HID report descriptors */
/**
 * @brief Simple HID mouse report descriptor for n button mouse.
 *
 * @param bcnt	Button count. Allowed values from 1 to 8.
 */
#define HID_GAMEPAD_REPORT_DESC() {				\
	/* USAGE_PAGE (Generic Desktop) */			\
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,			\
	/* USAGE (Game Pad) */					\
	HID_LI_USAGE, USAGE_GEN_DESKTOP_GAMEPAD,			\
	/* COLLECTION (Application) */				\
	HID_MI_COLLECTION, COLLECTION_APPLICATION,		\
		/* COLLECTION (Physical) */			\
		HID_MI_COLLECTION, COLLECTION_PHYSICAL,		\
			/* Bits used for button signalling */	\
			/* USAGE_PAGE (Button) */		\
			HID_GI_USAGE_PAGE, USAGE_GEN_BUTTON,	\
			/* USAGE_MINIMUM (Button 1) */		\
			HID_LI_USAGE_MIN(1), 0x01,		\
			/* USAGE_MAXIMUM (Button bcnt) */	\
			HID_LI_USAGE_MAX(1), 0x08,		\
			/* LOGICAL_MINIMUM (0) */		\
			HID_GI_LOGICAL_MIN(1), 0x00,		\
			/* LOGICAL_MAXIMUM (1) */		\
			HID_GI_LOGICAL_MAX(1), 0x01,		\
			/* REPORT_SIZE (1 bit) */			\
			HID_GI_REPORT_SIZE, 0x01,		\
			/* REPORT_COUNT (8 reports) */		\
			HID_GI_REPORT_COUNT, 0x08,		\
			/* INPUT (Data,Var,Abs) */		\
			HID_MI_INPUT, 0x02,			\
			/* X, Y, RX, RY */		\
			/* USAGE_PAGE (Generic Desktop) */	\
			HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,	\
			/* USAGE (X) */				\
			HID_LI_USAGE, USAGE_GEN_DESKTOP_X,	\
			/* USAGE (Y) */				\
			HID_LI_USAGE, USAGE_GEN_DESKTOP_Y,	\
			/* USAGE (RX) */				\
			HID_LI_USAGE, 0x33,	\
			/* USAGE (RY) */				\
			HID_LI_USAGE, 0x34,	\
			/* LOGICAL_MINIMUM (-127) */		\
			HID_GI_LOGICAL_MIN(1), -127,		\
			/* LOGICAL_MAXIMUM (127) */		\
			HID_GI_LOGICAL_MAX(1), 127,		\
			/* REPORT_SIZE (8) */			\
			HID_GI_REPORT_SIZE, 0x08,		\
			/* REPORT_COUNT (3) */			\
			HID_GI_REPORT_COUNT, 0x04,		\
			/* INPUT (Data,Var,Abs) */		\
			HID_MI_INPUT, 0x02,			\
		/* END_COLLECTION */				\
		HID_MI_COLLECTION_END,				\
	/* END_COLLECTION */					\
	HID_MI_COLLECTION_END,					\
}

static const uint8_t hid_report_desc[] = HID_GAMEPAD_REPORT_DESC();

static uint8_t def_val[12];
static volatile uint8_t status[5];
static K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */
static struct gpio_callback callback[12];
static enum usb_dc_status_code usb_status;

#define GAMEPAD_BTN_REPORT_POS	0
#define GAMEPAD_X_REPORT_POS	1
#define GAMEPAD_Y_REPORT_POS	2
#define GAMEPAD_RX_REPORT_POS	3
#define GAMEPAD_RY_REPORT_POS	4

#define GAMEPAD_BTN_ONE			BIT(0)
#define GAMEPAD_BTN_TWO			BIT(1)
#define GAMEPAD_BTN_THREE		BIT(2)
#define GAMEPAD_BTN_FOUR		BIT(3)
#define GAMEPAD_BTN_FIVE		BIT(4)
#define GAMEPAD_BTN_SIX			BIT(5)
#define GAMEPAD_BTN_SEVEN		BIT(6)
#define GAMEPAD_BTN_EIGHT		BIT(7)



static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

static void button_press(const struct device *gpio, struct gpio_callback *cb,
			uint32_t pins)
{
	// pins is BIT(pin) of the actual pin number. need to that log2 to get the
	// actual pin number. This then needs to get translated to gamepad button
	// number
	LOG_INF("BUTTON PRESSED!!! %u, %u, %lu, %lu, %i", pins, cb->pin_mask,
			GAMEPAD_BTN_ONE, (ulong_t)log2(pins), PIN0);
	int ret;
	uint8_t state = status[GAMEPAD_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	// Ensure only 1 pin is in this callback
	if (is_power_of_two(pins) == false) {
		LOG_ERR("More than one pin in button callback at a time");
	}

	// Get the state of the button
	// assumes only 1 pin is returned in the callback
	// maybe need to put a check that pins is a clean power of 2
	// or do something with the cb->pin_mask, but that only works if only 1 pin
	// is on the callback
	ret = gpio_pin_get(gpio, log2(pins));
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %d, error: %d",
			(int)log2(pins), ret);
		return;
	}
	LOG_INF("gpio_pin_get has value: %d", ret);

	// Determine the button pressed
	int btn = 0;
	switch ((int)log2(pins)) {
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
		case PIN0: btn = GAMEPAD_BTN_ONE; break;
#endif
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
		case PIN1: btn = GAMEPAD_BTN_TWO; break;
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
		case PIN2: btn = GAMEPAD_BTN_THREE; break;
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
		case GAMEPAD_BTN_FOUR: btn = GAMEPAD_BTN_FOUR; break;
#endif
#if DT_NODE_HAS_STATUS(SW4_NODE, okay)
		case GAMEPAD_BTN_FIVE: btn = GAMEPAD_BTN_FIVE; break;
#endif
#if DT_NODE_HAS_STATUS(SW5_NODE, okay)
		case GAMEPAD_BTN_SIX: btn = GAMEPAD_BTN_SIX; break;
#endif
#if DT_NODE_HAS_STATUS(SW6_NODE, okay)
		case GAMEPAD_BTN_SEVEN: btn = GAMEPAD_BTN_SEVEN; break;
#endif
#if DT_NODE_HAS_STATUS(SW7_NODE, okay)
		case GAMEPAD_BTN_EIGHT: btn = GAMEPAD_BTN_EIGHT; break;
#endif
		default: LOG_ERR("pins doesn't match any known pins"); break;
	}

	LOG_INF("Button %d callback has been called", btn);

	// Set or reset the button state
	if (ret == 0) {
		state &= ~btn;
	} else {
		state |= btn;
	}

	if (status[GAMEPAD_BTN_REPORT_POS] != state) {
		status[GAMEPAD_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

/*
static void x_move(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	int ret;
	uint8_t state = status[GAMEPAD_X_REPORT_POS];

	ret = gpio_pin_get(gpio, PIN2);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			PIN2, ret);
		return;
	}

	if (def_val[2] != (uint8_t)ret) {
		state += 10U;
	}

	if (status[GAMEPAD_X_REPORT_POS] != state) {
		status[GAMEPAD_X_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
*/

int callbacks_configure(const struct device *gpio, uint32_t pin, int flags,
			gpio_callback_handler_t handler,
			struct gpio_callback *callback, uint8_t *val)
{
	int ret;

	if (!gpio) {
		LOG_ERR("Could not find PORT");
		return -ENXIO;
	}

	ret = gpio_pin_configure(gpio, pin, GPIO_INPUT | flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure pin %u, error: %d",
			pin, ret);
		return ret;
	}

	ret = gpio_pin_get(gpio, pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			pin, ret);
		return ret;
	}

	*val = (uint8_t)ret;

	gpio_init_callback(callback, handler, BIT(pin));
	ret = gpio_add_callback(gpio, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add the callback for pin %u, error: %d",
			pin, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(gpio, pin, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt for pin %u, error: %d",
			pin, ret);
		return ret;
	}

	return 0;
}

void main(void)
{
	int ret;
	uint8_t report[4] = { 0x00 };
	const struct device *led_dev, *hid_dev;

	led_dev = device_get_binding(LED_PORT);
	if (led_dev == NULL) {
		LOG_ERR("Cannot get LED");
		return;
	}

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return;
	}

	ret = gpio_pin_configure(led_dev, LED, GPIO_OUTPUT | LED_FLAGS);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return;
	}

	if (callbacks_configure(device_get_binding(PORT0), PIN0, PIN0_FLAGS,
				&button_press, &callback[0], &def_val[0])) {
		LOG_ERR("Failed configuring left button callback.");
		return;
	}

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&button_press, &callback[1], &def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return;
	}
#endif

/*
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	if (callbacks_configure(device_get_binding(PORT2), PIN2, PIN2_FLAGS,
				&x_move, &callback[2], &def_val[2])) {
		LOG_ERR("Failed configuring X axis movement callback.");
		return;
	}
#endif

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	if (callbacks_configure(device_get_binding(PORT3), PIN3, PIN3_FLAGS,
				&y_move, &callback[3], &def_val[3])) {
		LOG_ERR("Failed configuring Y axis movement callback.");
		return;
	}
#endif
*/

	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				NULL);

	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	while (true) {
		k_sem_take(&sem, K_FOREVER);

		report[GAMEPAD_BTN_REPORT_POS] = status[GAMEPAD_BTN_REPORT_POS];
/*
		report[MOUSE_BTN_REPORT_POS] = status[MOUSE_BTN_REPORT_POS];
		report[MOUSE_X_REPORT_POS] = status[MOUSE_X_REPORT_POS];
		status[MOUSE_X_REPORT_POS] = 0U;
		report[MOUSE_Y_REPORT_POS] = status[MOUSE_Y_REPORT_POS];
		status[MOUSE_Y_REPORT_POS] = 0U;
*/
		ret = hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
		if (ret) {
			LOG_ERR("HID write error, %d", ret);
		}

		/* Toggle LED on sent report */
		ret = gpio_pin_toggle(led_dev, LED);
		if (ret < 0) {
			LOG_ERR("Failed to toggle the LED pin, error: %d", ret);
		}
	}
}
