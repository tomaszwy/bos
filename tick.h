#pragma once

#pragma pack(push, 1)
struct Tick {
  char symbol[16];
  double bid;
  double ask;
};
#pragma pack(pop)
