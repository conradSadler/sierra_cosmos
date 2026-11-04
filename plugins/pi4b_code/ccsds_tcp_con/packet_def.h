#include <stdint.h>

#define CCSDS_APID 1
#define PACKET_VERSION_BIT_LENGTH 3
#define APID_BIT_LENGTH 11
#define SEQUENCE_FLAG_BIT_LENGTH 2
#define PACKET_SEQUENCE_COUNT_BIT_LENGTH 14

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
  uint32_t  cpu_temp;    // millidegrees C
  
} payload_t;