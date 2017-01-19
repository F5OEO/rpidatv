#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <wiringPi.h>
#include "adf4351.h"

adf4350_init_param MyAdf=
{.clkin=25000000,
.channel_spacing=5000,
.power_up_frequency=437000000,
.reference_div_factor=0,
.reference_doubler_enable=0,
.reference_div2_enable=0,

	// r2_user_settings 
.phase_detector_polarity_positive_enable=1,
.lock_detect_precision_6ns_enable=0,
.lock_detect_function_integer_n_enable=0,
.charge_pump_current=7, // FixMe
.muxout_select=0,
.low_spur_mode_enable=1,

	// r3_user_settings 
.cycle_slip_reduction_enable=1,
.charge_cancellation_enable=0,
.anti_backlash_3ns_enable=0,
.band_select_clock_mode_high_enable=1,
.clk_divider_12bit=0,
.clk_divider_mode=0,

	// r4_user_settings 
.aux_output_enable=1,
.aux_output_fundamental_enable=1,
.mute_till_lock_enable=0,
.output_power=0,//-4dbm
.aux_output_power=0

};
uint32_t registers[6] =  {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC803C, 0x580005};
uint32_t power = 0;
//REG 0
//REG1 1000000000001000000011001 001
//REG2 100111001000 010
//REG3 10010110 011
//REG4 101111001000000000111 100


/***************************************************************************//**
 * @brief Powers off or sets the Synth frequency.
 *
 * @param "off" or freq in MHz (float).
 *
 * @return 0 or 1 if freq out of bounds
*******************************************************************************/

int main(int argc, char *argv[])
{
    if (wiringPiSetup() == -1);

        if (strcmp(argv[1], "off") == 0)
        {
	    // Turn VCO Off and return

	    adf4350_out_altvoltage0_powerdown(1);

	    return 0;
	}
	else if ( atof(argv[1])>=35 && atof(argv[1])<=4400 )
	{
            // Valid freq, so set it
            uint32_t adf4350_requested_frequency = 1000000 * atof(argv[1]);

            if (argc==3 && atof(argv[2])>=0 && atof(argv[2])<=3 )
            {
                // VCO Power level specified, so set it
               power = atof(argv[2]);
            }
            else
            {
                // VCO Power level not specified or invalid so set to default of 0
                power = 0;
            }
            // Set up the ADF
            adf4350_setup(0,0,MyAdf);
            // Turn it on
            adf4350_out_altvoltage0_frequency(adf4350_requested_frequency);

            return 0;
	}
	else
	{
            // Requested freq out of limits so print error and return 1
            printf("ERROR: Requested Frequency out of limits");

            return 1;
	}
}
