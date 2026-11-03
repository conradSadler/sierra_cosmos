#include <stdint.h>

#define CCSDS_APID 1

// payload_t length is 48 bits for header and 32 bits for data to 80 bits / 10 bytes
typedef struct __attribute__((packed)) {
    uint16_t packet_id;   // version(3) | type(1) | sec hdr flag(1) | APID(11)
    uint16_t seq_ctrl;    // seq flags(2) | seq count(14)
    uint16_t length;      // data field bytes - 1
    uint32_t  cpu_temp;    // millidegrees C
} payload_t;