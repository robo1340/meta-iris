/*
// Command:                  RF_POWER_UP
// Description:              Command to power-up the device and select the operational mode and functionality.
*/
#define RF_POWER_UP 0x02, 0x81, 0x00, 0x01, 0xC9, 0xC3, 0x80

/*
// Command:                  RF_GPIO_PIN_CFG
// Description:              Configures the GPIO pins.
*/
#define RF_GPIO_PIN_CFG 0x13, 2, 3, 0x21, 0x20, 0x00, 0x00, 0x00

/*
// Set properties:           RF_GLOBAL_XO_TUNE_2
// Number of properties:     2
// Group ID:                 0x00
// Start ID:                 0x00
// Default values:           0x40, 0x00, 
// Descriptions:
//   GLOBAL_XO_TUNE - Configure the internal capacitor frequency tuning bank for the crystal oscillator.
//   GLOBAL_CLK_CFG - Clock configuration options.
*/
#define RF_GLOBAL_XO_TUNE_2 0x11, 0x00, 0x02, 0x00, 0x52, 0x00

/*
// Set properties:           RF_GLOBAL_CONFIG_1
// Number of properties:     1
// Group ID:                 0x00
// Start ID:                 0x03
// Default values:           0x20, 
// Descriptions:
//   GLOBAL_CONFIG - Global configuration settings.
*/
#define RF_GLOBAL_CONFIG_1 0x11, 0x00, 0x01, 0x03, 0x30

/*
// Set properties:           RF_INT_CTL_ENABLE_4
// Number of properties:     4
// Group ID:                 0x01
// Start ID:                 0x00
// Default values:           0x04, 0x00, 0x00, 0x04, 
// Descriptions:
//   INT_CTL_ENABLE - This property provides for global enabling of the three interrupt groups (Chip, Modem and Packet Handler) in order to generate HW interrupts at the NIRQ pin.
//   INT_CTL_PH_ENABLE - Enable individual interrupt sources within the Packet Handler Interrupt Group to generate a HW interrupt on the NIRQ output pin.
//   INT_CTL_MODEM_ENABLE - Enable individual interrupt sources within the Modem Interrupt Group to generate a HW interrupt on the NIRQ output pin.
//   INT_CTL_CHIP_ENABLE - Enable individual interrupt sources within the Chip Interrupt Group to generate a HW interrupt on the NIRQ output pin.
*/
#define RF_INT_CTL_ENABLE_4 0x11, 0x01, 0x04, 0x00, 0x07, 0x33, 0x01, 0x30

/*
// Set properties:           RF_FRR_CTL_A_MODE_4
// Number of properties:     4
// Group ID:                 0x02
// Start ID:                 0x00
// Default values:           0x01, 0x02, 0x09, 0x00, 
// Descriptions:
//   FRR_CTL_A_MODE - Fast Response Register A Configuration. (0x09==current state)
//   FRR_CTL_B_MODE - Fast Response Register B Configuration.
//   FRR_CTL_C_MODE - Fast Response Register C Configuration.
//   FRR_CTL_D_MODE - Fast Response Register D Configuration.
*/
#define RF_FRR_CTL_A_MODE_4 0x11, 0x02, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00