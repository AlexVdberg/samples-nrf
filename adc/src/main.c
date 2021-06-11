/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/adc.h>
#include <sys/util.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NUM_CHANNELS	DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)

#if ADC_NUM_CHANNELS != 8
#error "Currently only 8 channels supported in this sample"
#endif

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION			12
#define ADC_GAIN				ADC_GAIN_1_6
#define ADC_REFERENCE			ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT

/* Get the numbers of all 8 channels */
static uint8_t channel_ids[ADC_NUM_CHANNELS] = {
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 1),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 2),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 3),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 4),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 5),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 6),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 7)
};

static int16_t sample_buffer[ADC_NUM_CHANNELS];

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	/* channel ID will be overwritten below */
	.channel_id = 0,
	.differential = 0
};

struct adc_sequence sequence = {
	/* individual channels will be added below */
	.channels    = 0,
	.buffer      = sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

void main(void)
{
	int err;
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);
	const struct device *dev = device_get_binding(
			CONFIG_UART_CONSOLE_ON_DEV_NAME);
	uint32_t dtr = 0;

	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return;
	}

	if (usb_enable(NULL)) {
		return;
	}

	/* Poll if the DTR flag was set, optional */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
	}

	if (strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME) !=
	    strlen("CDC_ACM_0") ||
	    strncmp(CONFIG_UART_CONSOLE_ON_DEV_NAME, "CDC_ACM_0",
		    strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME))) {
		printk("Error: Console device name is not USB ACM\n");

		return;
	}

	/*
	 * Configure channels individually prior to sampling
	 */
	for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
		channel_cfg.channel_id = channel_ids[i];
#ifdef CONFIG_ADC_NRFX_SAADC
		channel_cfg.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0
					     + channel_ids[i];
#endif

		adc_channel_setup(dev_adc, &channel_cfg);

		sequence.channels |= BIT(channel_ids[i]);
	}

	int32_t adc_vref = adc_ref_internal(dev_adc);

	while (1) {
		/*
		 * Read sequence of channels (fails if not supported by MCU)
		 */
		err = adc_read(dev_adc, &sequence);
		if (err != 0) {
			printk("ADC reading failed with error %d.\n", err);
			return;
		}

		for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
			printk("Channel %d:", channel_ids[i]);
			int32_t raw_value = sample_buffer[i];

			printk(" %d", raw_value);
			if (adc_vref > 0) {
				/*
				 * Convert raw reading to millivolts if driver
				 * supports reading of ADC reference voltage
				 */
				int32_t mv_value = raw_value;

				adc_raw_to_millivolts(adc_vref, ADC_GAIN,
					ADC_RESOLUTION, &mv_value);
				printk(" = %d mV  ", mv_value);
			}
		}
		printk("\n");

		printk("ADC_GAIN: %d, ADC_RESOLUTION %d, ADC_NUM_CHANNELS %d",
				ADC_GAIN,
				ADC_RESOLUTION,
				ADC_NUM_CHANNELS);
		printk(", ADC_REFERENCE %d\n",
				ADC_REFERENCE);

		k_sleep(K_MSEC(1000));
	}
}
