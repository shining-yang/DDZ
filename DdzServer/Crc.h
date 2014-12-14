//
// Crc.h
//
#pragma once

// 计算16-bit的CRC，使用标准CRC-16多项式
unsigned short CRC16(unsigned char* aData, unsigned long aSize);

static void BuildTable16(unsigned short aPoly);
