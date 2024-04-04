#include "j1939.h"

J1939Header parseHeader(uint32_t id) {
  J1939Header header;
  header.priority = id & 0x1C000000;
  header.pgn = id & 0x00FFFF00;
  header.pgn >>= 8;
  header.src = static_cast<int>(id);

  if (isPeerToPeer(&header)) {
    header.dest = static_cast<int>(header.pgn & 0xFF);
    header.pgn &= 0x01FF00;
  }

  return header;
}

bool isPeerToPeer(J1939Header *header) {
  // Check the PGN
  if (header->pgn > 0 && header->pgn <= 0xEFFF)
    return true;

  if (header->pgn > 0x10000 && header->pgn <= 0x1EFFF)
    return true;

  return false;
}