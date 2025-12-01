/*
// Set properties:           RF_PREAMBLE_TX_LENGTH_9
// Number of properties:     9
// Group ID:                 0x10
// Start ID:                 0x00
// Default values:           0x08, 0x14, 0x00, 0x0F, 0x21, 0x00, 0x00, 0x00, 0x00, 
// Descriptions:
//   PREAMBLE_TX_LENGTH - Configure length of TX Preamble.
//   PREAMBLE_CONFIG_STD_1 - Configuration of reception of a packet with a Standard Preamble pattern.
//   PREAMBLE_CONFIG_NSTD - Configuration of transmission/reception of a packet with a Non-Standard Preamble pattern.
//   PREAMBLE_CONFIG_STD_2 - Configuration of timeout periods during reception of a packet with Standard Preamble pattern.
//   PREAMBLE_CONFIG - General configuration bits for the Preamble field.
//   PREAMBLE_PATTERN_31_24 - Configuration of the bit values describing a Non-Standard Preamble pattern.
//   PREAMBLE_PATTERN_23_16 - Configuration of the bit values describing a Non-Standard Preamble pattern.
//   PREAMBLE_PATTERN_15_8 - Configuration of the bit values describing a Non-Standard Preamble pattern.
//   PREAMBLE_PATTERN_7_0 - Configuration of the bit values describing a Non-Standard Preamble pattern.
*/

//much shorter
#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0x0F, 0x18, 0x00, 0xEF, 0x2D, 0x00, 0x00, 0x00, 0x00

//four times shorter
//#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0x7F, 0x14, 0x00, 0xEF, 0x2D, 0x00, 0x00, 0x00, 0x00

//longer
//#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0xFF, 0x14, 0x00, 0xEF, 0x3D, 0x00, 0x00, 0x00, 0x00

//original
//#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0xFF, 0x94, 0x00, 0xEF, 0x31, 0x00, 0x00, 0x00, 0x00

/*
// Set properties:           RF_SYNC_CONFIG_6
// Number of properties:     6
// Group ID:                 0x11
// Start ID:                 0x00
// Default values:           0x01, 0x2D, 0xD4, 0x2D, 0xD4, 0x00, 
// Descriptions:
//   SYNC_CONFIG - Sync Word configuration bits.
//   SYNC_BITS_31_24 - Sync word.
//   SYNC_BITS_23_16 - Sync word.
//   SYNC_BITS_15_8 - Sync word.
//   SYNC_BITS_7_0 - Sync word.
//   SYNC_CONFIG2 - Sync Word configuration bits.
*/

//known working
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x02, 0x45, 0xD2, 0xCC, 0x00, 0x00

//barker code 1 error forgiven
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x12, 0x27, 0x27, 0x27, 0x00, 0x00

//1 byte
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x00, 0x27, 0x27, 0x27, 0x00, 0x00

//manchester codes
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x06, 0x45, 0xD2, 0xCC, 0x00, 0x00

//1 byte manchester codes
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x04, 0x27, 0x27, 0x27, 0x00, 0x00
#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x34, 0x27, 0x27, 0x27, 0x00, 0x00

//3 byte manchester codes
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x06, 0x27, 0x27, 0x27, 0x00, 0x00

//4 byte manchester codes
//#define RF_SYNC_CONFIG_6 0x11, 0x11, 0x06, 0x00, 0x07, 0x27, 0x27, 0x27, 0x27, 0x00
