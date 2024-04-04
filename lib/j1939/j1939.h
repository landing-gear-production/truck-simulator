#include <stdint.h>

struct J1939Header {
  uint8_t priority;
  uint32_t pgn;
  uint8_t src;
  uint8_t dest;
};

J1939Header parseHeader(uint32_t id);
bool isPeerToPeer(J1939Header *header);