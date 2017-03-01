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

uint32_t registers[6] =  {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC803C, 0x580005};

//REG0 0100 0101 1000 0000 1010 1000
//REG1 1000 0000 0000 1000 0000 1100 1001
//REG2 0100 1110 0100 0010
//REG3 0100 1011 0011
//REG4 1011 1100 1000 0000 0011 1100
//REG5 0101 1000 0000 0000 0000 0101

/***************************************************************************//**
 * @brief Powers off or sets the Synth ref freq, output freq and power level.
 *
 * @param "off" or freq in MHz (float), optional ref freq in Hz and optional power 0 - 3
 *
 * @return 0 or 1 if freq out of bounds
*******************************************************************************/

int main(int argc, char *argv[])
{
  // Kick Wiring Pi into life
  if (wiringPiSetup() == -1);

  // set parameter defaults
  uint32_t adf4350_requested_frequency = 1255000000;
  uint32_t adf4350_requested_ref_freq = 25000000;
  uint16_t adf4350_requested_power = 0;

  // Check first parameter
  if (argc==1 || strcmp(argv[1], "off") == 0)
  {
    // No frequency specified or turnoff requested
    // Turn VCO Off and return

    adf4350_out_altvoltage0_powerdown(1);
    return 0;
  }

  else if ( atof(argv[1])>=35 && atof(argv[1])<=4400 )
  {
    // Valid freq, so set it
    adf4350_requested_frequency = 1000000 * atof(argv[1]);

    // Check second parameter
    if (argc>=3 && atof(argv[2])>=4995000 && atof(argv[2])<=25010000)
    {
      // valid reference osc freq, so set it
      adf4350_requested_ref_freq = atof(argv[2]);
    }

    // Check third parameter
    if (argc>=4 && atof(argv[3])>=0 && atof(argv[3])<=3 )
    {
      // Valid VCO Power level specified, so set it
      adf4350_requested_power = atof(argv[3]);
    }

    // Valid input so set parameters
    adf4350_init_param MyAdf=
    {
      // Calculation inputs
      .clkin=adf4350_requested_ref_freq,
      .channel_spacing=5000,
      .power_up_frequency=adf4350_requested_frequency,
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
      .aux_output_enable=0,
      .aux_output_fundamental_enable=1,
      .mute_till_lock_enable=1,
      .output_power=adf4350_requested_power,
      .aux_output_power=0
    };

    // Send the commands to the ADF4351
    adf4350_setup(0,0,MyAdf);
    return 0;
  }
  else
  {
    // Requested freq out of limits so print error and return 1
    printf("ERROR: Requested Frequency out of limits");
    return 1;
  }
}
