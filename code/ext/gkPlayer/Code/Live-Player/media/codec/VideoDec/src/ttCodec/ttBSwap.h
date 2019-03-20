#ifndef __TTPOD_TT_BSWAP_H_
#define __TTPOD_TT_BSWAP_H_

#define TTV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define TTV_BSWAP32C(x) (TTV_BSWAP16C(x) << 16 | TTV_BSWAP16C((x) >> 16))
#define TTV_BSWAP64C(x) (TTV_BSWAP32C(x) << 32 | TTV_BSWAP32C((x) >> 32))

#define TTV_BSWAPC(s, x) TTV_BSWAP##s##C(x)

#ifndef ttv_bswap16
static ttv_always_inline ttv_const uint16_t ttv_bswap16(uint16_t x)
{
	x= (x>>8) | (x<<8);
	return x;
}
#endif

#ifndef ttv_bswap32
static ttv_always_inline ttv_const uint32_t ttv_bswap32(uint32_t x)
{
	return TTV_BSWAP32C(x);
}
#endif

#ifndef ttv_bswap64
static inline uint64_t ttv_const ttv_bswap64(uint64_t x)
{
	return (uint64_t)ttv_bswap32(x) << 32 | ttv_bswap32(x >> 32);
}
#endif

#endif //__TTPOD_TT_BSWAP_H_

