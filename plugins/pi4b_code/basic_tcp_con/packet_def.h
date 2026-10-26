nclude <stdint.h>

#define SYNC_PATTERN 0xABCD1234

typedef struct __attribute__((packed)) {
	    uint8_t id;
	        uint32_t cpu_tmp;
} payload_t;
