/*
// Set properties:           RF_PKT_CRC_CONFIG_7
// Number of properties:     7
// Group ID:                 0x12
// Start ID:                 0x00
// Default values:           0x00, 0x01, 0x08, 0xFF, 0xFF, 0x00, 0x00, 
// Descriptions:
//   PKT_CRC_CONFIG - Select a CRC polynomial and seed.
//   PKT_WHT_POLY_15_8 - 16-bit polynomial value for the PN Generator (e.g., for Data Whitening)
//   PKT_WHT_POLY_7_0 - 16-bit polynomial value for the PN Generator (e.g., for Data Whitening)
//   PKT_WHT_SEED_15_8 - 16-bit seed value for the PN Generator (e.g., for Data Whitening)
//   PKT_WHT_SEED_7_0 - 16-bit seed value for the PN Generator (e.g., for Data Whitening)
//   PKT_WHT_BIT_NUM - Selects which bit of the LFSR (used to generate the PN / data whitening sequence) is used as the output bit for data scrambling.
//   PKT_CONFIG1 - General configuration bits for transmission or reception of a packet.
*/
#define RF_PKT_CRC_CONFIG_7 0x11, 0x12, 0x07, 0x00, 0x80, 0x01, 0x08, 0xFF, 0xFF, 0x00, 0x02

/*
// Set properties:           RF_PKT_LEN_12
// Number of properties:     12
// Group ID:                 0x12
// Start ID:                 0x08
// Default values:           0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// Descriptions:
//   PKT_LEN - Configuration bits for reception of a variable length packet.
//   PKT_LEN_FIELD_SOURCE - Field number containing the received packet length byte(s).
//   PKT_LEN_ADJUST - Provides for adjustment/offset of the received packet length value (in order to accommodate a variety of methods of defining total packet length).
//   PKT_TX_THRESHOLD - TX FIFO almost empty threshold.
//   PKT_RX_THRESHOLD - RX FIFO Almost Full threshold.
//   PKT_FIELD_1_LENGTH_12_8 - Unsigned 13-bit Field 1 length value.
//   PKT_FIELD_1_LENGTH_7_0 - Unsigned 13-bit Field 1 length value.
//   PKT_FIELD_1_CONFIG - General data processing and packet configuration bits for Field 1.
//   PKT_FIELD_1_CRC_CONFIG - Configuration of CRC control bits across Field 1.
//   PKT_FIELD_2_LENGTH_12_8 - Unsigned 13-bit Field 2 length value.
//   PKT_FIELD_2_LENGTH_7_0 - Unsigned 13-bit Field 2 length value.
//   PKT_FIELD_2_CONFIG - General data processing and packet configuration bits for Field 2.
*/
//#define RF_PKT_LEN_12 0x11, 0x12, 0x0C, 0x08, 0x00, 0x00, 0x00, 64, 64, 0x09, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00
#define RF_PKT_LEN_12 0x11, 0x12, 0x0C, 0x08, 0x00, 0x00, 0x00, 64, 64, 0x04, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// Set properties:           RF_PKT_FIELD_2_CRC_CONFIG_12
// Number of properties:     12
// Group ID:                 0x12
// Start ID:                 0x14
// Default values:           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// Descriptions:
//   PKT_FIELD_2_CRC_CONFIG - Configuration of CRC control bits across Field 2.
//   PKT_FIELD_3_LENGTH_12_8 - Unsigned 13-bit Field 3 length value.
//   PKT_FIELD_3_LENGTH_7_0 - Unsigned 13-bit Field 3 length value.
//   PKT_FIELD_3_CONFIG - General data processing and packet configuration bits for Field 3.
//   PKT_FIELD_3_CRC_CONFIG - Configuration of CRC control bits across Field 3.
//   PKT_FIELD_4_LENGTH_12_8 - Unsigned 13-bit Field 4 length value.
//   PKT_FIELD_4_LENGTH_7_0 - Unsigned 13-bit Field 4 length value.
//   PKT_FIELD_4_CONFIG - General data processing and packet configuration bits for Field 4.
//   PKT_FIELD_4_CRC_CONFIG - Configuration of CRC control bits across Field 4.
//   PKT_FIELD_5_LENGTH_12_8 - Unsigned 13-bit Field 5 length value.
//   PKT_FIELD_5_LENGTH_7_0 - Unsigned 13-bit Field 5 length value.
//   PKT_FIELD_5_CONFIG - General data processing and packet configuration bits for Field 5.
*/
#define RF_PKT_FIELD_2_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// Set properties:           RF_PKT_FIELD_5_CRC_CONFIG_12
// Number of properties:     12
// Group ID:                 0x12
// Start ID:                 0x20
// Default values:           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// Descriptions:
//   PKT_FIELD_5_CRC_CONFIG - Configuration of CRC control bits across Field 5.
//   PKT_RX_FIELD_1_LENGTH_12_8 - Unsigned 13-bit RX Field 1 length value.
//   PKT_RX_FIELD_1_LENGTH_7_0 - Unsigned 13-bit RX Field 1 length value.
//   PKT_RX_FIELD_1_CONFIG - General data processing and packet configuration bits for RX Field 1.
//   PKT_RX_FIELD_1_CRC_CONFIG - Configuration of CRC control bits across RX Field 1.
//   PKT_RX_FIELD_2_LENGTH_12_8 - Unsigned 13-bit RX Field 2 length value.
//   PKT_RX_FIELD_2_LENGTH_7_0 - Unsigned 13-bit RX Field 2 length value.
//   PKT_RX_FIELD_2_CONFIG - General data processing and packet configuration bits for RX Field 2.
//   PKT_RX_FIELD_2_CRC_CONFIG - Configuration of CRC control bits across RX Field 2.
//   PKT_RX_FIELD_3_LENGTH_12_8 - Unsigned 13-bit RX Field 3 length value.
//   PKT_RX_FIELD_3_LENGTH_7_0 - Unsigned 13-bit RX Field 3 length value.
//   PKT_RX_FIELD_3_CONFIG - General data processing and packet configuration bits for RX Field 3.
*/
#define RF_PKT_FIELD_5_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// Set properties:           RF_PKT_RX_FIELD_3_CRC_CONFIG_9
// Number of properties:     9
// Group ID:                 0x12
// Start ID:                 0x2C
// Default values:           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// Descriptions:
//   PKT_RX_FIELD_3_CRC_CONFIG - Configuration of CRC control bits across RX Field 3.
//   PKT_RX_FIELD_4_LENGTH_12_8 - Unsigned 13-bit RX Field 4 length value.
//   PKT_RX_FIELD_4_LENGTH_7_0 - Unsigned 13-bit RX Field 4 length value.
//   PKT_RX_FIELD_4_CONFIG - General data processing and packet configuration bits for RX Field 4.
//   PKT_RX_FIELD_4_CRC_CONFIG - Configuration of CRC control bits across RX Field 4.
//   PKT_RX_FIELD_5_LENGTH_12_8 - Unsigned 13-bit RX Field 5 length value.
//   PKT_RX_FIELD_5_LENGTH_7_0 - Unsigned 13-bit RX Field 5 length value.
//   PKT_RX_FIELD_5_CONFIG - General data processing and packet configuration bits for RX Field 5.
//   PKT_RX_FIELD_5_CRC_CONFIG - Configuration of CRC control bits across RX Field 5.
*/
#define RF_PKT_RX_FIELD_3_CRC_CONFIG_9 0x11, 0x12, 0x09, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
