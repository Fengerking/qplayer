/*******************************************************************************
File:		bit_t.h

Contains:	the base parser header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#ifndef __QP_BIT_T_H__
#define __QP_BIT_T_H__

#include "cmbasetype.h"
#include "string.h"

typedef uint8 bit1;
typedef uint8 bit2;
typedef uint8 bit3;
typedef uint8 bit4;
typedef uint8 bit5;
typedef uint8 bit6;
typedef uint8 bit7;
typedef uint8 bit8;
typedef uint16 bit9;
typedef uint16 bit10;
typedef uint16 bit11;
typedef uint16 bit12;
typedef uint16 bit13;
typedef uint16 bit14;
typedef uint16 bit15;
typedef uint16 bit16;
typedef uint32 bit17;
typedef uint32 bit18;
typedef uint32 bit19;
typedef uint32 bit20;
typedef uint32 bit21;
typedef uint32 bit22;
typedef uint32 bit23;
typedef uint32 bit24;
typedef uint32 bit25;
typedef uint32 bit26;
typedef uint32 bit27;
typedef uint32 bit28;
typedef uint32 bit29;
typedef uint32 bit30;
typedef uint32 bit31;
typedef uint32 bit32;
typedef uint64 bit33;
typedef uint64 bit34;
typedef uint64 bit35;
typedef uint64 bit36;
typedef uint64 bit40;
typedef uint64 bit64;

#define R1B7(d) (d >> 7)
#define R1B6(d) ((d >> 6) & 0x01)
#define R1B5(d) ((d >> 5) & 0x01)
#define R1B4(d) ((d >> 4) & 0x01)
#define R1B3(d) ((d >> 3) & 0x01)
#define R1B2(d) ((d >> 2) & 0x01)
#define R1B1(d) ((d >> 1) & 0x01)
#define R1B0(d) (d & 0x01)

#define R2B7(d) (d >> 6)
#define R2B6(d) ((d >> 5) & 0x03)
#define R2B5(d) ((d >> 4) & 0x03)
#define R2B4(d) ((d >> 3) & 0x03)
#define R2B3(d) ((d >> 2) & 0x03)
#define R2B2(d) ((d >> 1) & 0x03)
#define R2B1(d) (d & 0x03)

#define R3B7(d) (d >> 5)
#define R3B6(d) ((d >> 4) & 0x07)
#define R3B5(d) ((d >> 3) & 0x07)
#define R3B4(d) ((d >> 2) & 0x07)
#define R3B3(d) ((d >> 1) & 0x07)
#define R3B2(d) (d & 0x07)

#define R4B7(d) (d >> 4)
#define R4B6(d) ((d >> 3) & 0x0f)
#define R4B5(d) ((d >> 2) & 0x0f)
#define R4B4(d) ((d >> 1) & 0x0f)
#define R4B3(d) (d & 0x0f)

#define R5B7(d) (d >> 3)
#define R5B6(d) ((d >> 2) & 0x1f)
#define R5B5(d) ((d >> 1) & 0x1f)
#define R5B4(d) (d & 0x1f)

#define R6B7(d) (d >> 2)
#define R6B6(d) ((d >> 1) & 0x3f)
#define R6B5(d) (d & 0x3f)

#define R7B7(d) (d >> 1)
#define R7B6(d) (d & 0x7f)

#define R16B(p, d) *(((uint8*)&d) + 1) = *p++; *(uint8*)&d = *p++
#define R24B(p, d) *(((uint8*)&d) + 3) = 0; *(((uint8*)&d) + 2) = *p++; *(((uint8*)&d) + 1) = *p++; *(uint8*)&d = *p++
#define R32B(p, d) *(((uint8*)&d) + 3) = *p++; *(((uint8*)&d) + 2) = *p++; *(((uint8*)&d) + 1) = *p++; *(uint8*)&d = *p++

inline void R12B(uint8*& p, uint16& d)
{
	d = 0x0f & *p++;
	d <<= 8;
	d |= *p++;
}

inline void R13B(uint8*& p, uint16& d)
{
	d = 0x1f & *p++;
	d <<= 8;
	d |= *p++;
}

inline void R22B(uint8*& p, uint32& d)
{
	d = R7B6(*p++);
	d <<= 8;
	d |= *p++;
	d <<= 7;
	d |= R7B7(*p++) >> 1;
}

inline void R33B(uint8*& p, uint64& ts)
{
	ts = *p++;
	ts &= 0x0e;
	ts <<= 7;
	ts |= *p++;
	ts <<= 8;
	ts |= (*p++) & 0xfe;
	ts <<= 7;
	ts |= *p++;
	ts <<= 7;
	ts |= ((*p++) & 0xfe) >> 1;
}

inline void R33n10B(uint8*& p, uint64& base, uint16& ext)
{
	base = R3B5(*p); 
	base |= R2B1(*p++) << 1;
	base <<= 7;
	base |= *p++;
	base <<= 8;
	base |= R5B7(*p);
	base <<= R2B1(*p++) << 1;
	base <<= 7;
	base |= *p++;
	base <<= 5;
	base |= R5B7(*p) >> 3;
	ext = R2B1(*p++);
	ext <<= 7;
	ext |= R7B7(*p++) >> 1;
}

inline void RBytes(uint8*& p, uint32& data, int n)
{
	data = 0;
	for (int i = 0; i < n; i++)
	{
		data <<= 8;
		data += *p++;
	}
}


const uint8 BITVALMASK[] = { 0, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

class BitStream
{
public:

	BitStream(uint8* p=0)
		: pbyte(p)
		, bitpos(8)
	{
	}

	void Init(uint8* p)
	{
		pbyte = p;
		bitpos = 8;
	}

	uint8* Position()
	{
		return pbyte;
	}

	void SetPosition(uint8* pos)
	{
		pbyte = pos;
		bitpos = 8;
	}

	void ReadData(int size, void* data)
	{
		memcpy(data, pbyte, size);
		pbyte += size;
	}

	void SkipBits(int width)
	{
		pbyte += width / 8;
		bitpos -= width % 8;
		if (bitpos <= 0)
		{
			pbyte++;
			bitpos += 8;
		}
	}

	void SkipBytes(int size)
	{
		pbyte += size;
	}

	void ReadBits0(int width, uint8& bs) // size <= 8
	{
		if (bitpos < width)
			return;
		bitpos -= width; 
		bs = (*pbyte >> bitpos) & BITVALMASK[width];
		if (bitpos == 0)
		{
			bitpos = 8;
			pbyte++;
		}
	}

	void ReadBits(int width, uint8& bs) // size <= 8
	{
		if (bitpos >= width)
		{
			ReadBits0(width, bs);
			return;
		}
		int width2 = width - bitpos;
		uint8 u2;
		ReadBits0(bitpos, bs);
		ReadBits0(width2, u2);
		bs <<= width2;
		bs |= u2;
	}

	void ReadBits(int width, uint16& bs) // 8 < size <= 16
	{
		bs = *pbyte & BITVALMASK[bitpos];
		int leftwidth = width - bitpos;
		pbyte++;
		bitpos = 8;
		while (leftwidth >= 8)
		{
			bs <<= 8;
			bs |= *pbyte++;
			leftwidth -= 8;
		}
		bitpos -= leftwidth;
		bs <<= leftwidth;
		bs |= (*pbyte >> bitpos) & BITVALMASK[leftwidth];
	}

	void ReadBits(int width, uint32& bs) // 8 < size <= 32
	{
		bs = *pbyte & BITVALMASK[bitpos];
		int leftwidth = width - bitpos;
		pbyte++;
		bitpos = 8;
		while (leftwidth >= 8)
		{
			bs <<= 8;
			bs |= *pbyte++;
			leftwidth -= 8;
		}
		bitpos -= leftwidth;
		bs <<= leftwidth;
		bs |= (*pbyte >> bitpos) & BITVALMASK[leftwidth];
	}

	void ReadBits(int width, uint64& bs) // 8 < size <= 64
	{
		bs = *pbyte & BITVALMASK[bitpos];
		int leftwidth = width - bitpos;
		pbyte++;
		bitpos = 8;
		while (leftwidth >= 8)
		{
			bs <<= 8;
			bs |= *pbyte++;
			leftwidth -= 8;
		}
		bitpos -= leftwidth;
		bs <<= leftwidth;
		bs |= (*pbyte >> bitpos) & BITVALMASK[leftwidth];
	}

	void ShiftAlign(uint8* pStart, int count)
	{
		uint8* p = pStart;
		while (count)
		{
			*p <<= 8 - bitpos;
			*p |= *(p + 1) >> bitpos;
			p++;
			count--;
		}
	}

private:
	uint8* pbyte;
	int bitpos;

};

#endif
