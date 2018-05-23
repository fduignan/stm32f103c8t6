#include <stdint.h>
int writeSector(uint32_t Address,void * valuePtr, uint16_t Count);
void eraseSector(uint32_t SectorStartAddress);
void readSector(uint32_t SectorStartAddress, void * valuePtr, uint16_t Count);
