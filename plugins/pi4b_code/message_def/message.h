/*
 * message.h
 *
 * 
 * Created on: Oct 27, 2026
 *     Author: Alan
*/

#ifndef INC_MESSAGE_H_
#define INC_MESSAGE_H_

#include <stdint.h>

#define   PACKET_VERSION_BIT_LENGTH 3
#define   APID_BIT_LENGTH 11
#define   SEQUENCE_FLAG_BIT_LENGTH 2
#define   PACKET_SEQUENCE_COUNT_BIT_LENGTH 14

#define   MAX_DATA_LENGTH 65536

/** @defgroup Message ID structures
  * @{
  */
typedef enum
{
  MESSAGE_ID_DEFAULT                            = 0U,
  MESSAGE_ID_POWER_ON                           = 1U,
  MESSAGE_ID_ID_EXCHANCE                        = 2U,
  MESSAGE_ID_TIME                               = 3U,
  MESSAGE_ID_STATE                              = 4U,
  MESSAGE_ID_OFFSET                             = 5U,
  MESSAGE_ID_DO_BIT                             = 6U,
  MESSAGE_ID_BIT_COMPLETE_STATUS                = 7U,
  MESSAGE_ID_TELEMETRY_UPDATE                   = 15U,
  MESSAGE_ID_SHUTDOWN                           = 16U,
  MESSAGE_ID_START_FILE_TRANSFER                = 17U,
  MESSAGE_ID_FILE_TRANSFER_COMPLETE_STATUS      = 18U,
  MESSAGE_ID_RETURN_TO_IDLE                     = 19U,
  MESSAGE_ID_RESENT_LAST_RESPONSE               = 20U
  // Append more as you needed
} Message_IDTypeDef;
/**
  * @}
  */

typedef struct __attribute__((packed)){
    // First two octets of the primary header
    // APID length is not long enough 11 bits compared to 16 bits
    uint8_t packet_version:PACKET_VERSION_BIT_LENGTH;
    uint8_t packet_type_flag:1;
    uint8_t secondary_header_flag:1;
    uint16_t application_process_identifier:APID_BIT_LENGTH; //Might change to message_idtype def

    // Second two octets of the primary header
    uint8_t sequence_flags:SEQUENCE_FLAG_BIT_LENGTH;
    uint16_t packet_sequence_count:PACKET_SEQUENCE_COUNT_BIT_LENGTH;

    // Last two octets of the primary header
    uint16_t packet_data_length;
} message_header_t;

// Commented out since each message definition is two widely varied.
// typedef struct __attribute__((packed)){
//     message_header_t primary_header;
//     uint8_t data[MAX_DATA_LENGTH];
// } message_t;
    
#endif