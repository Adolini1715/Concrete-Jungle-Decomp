namespace ex {
typedef unsigned char u8; typedef unsigned int u32;
struct RGBA {
    u8 r, g, b, a;
    RGBA();
    RGBA(u8 r_, u8 g_, u8 b_, u8 a_);
    operator u32() const;
};
RGBA::RGBA() {}
RGBA::RGBA(u8 r_, u8 g_, u8 b_, u8 a_) { r = r_; g = g_; b = b_; a = a_; }
RGBA::operator u32() const { return *(const u32 *)this; }
struct EXBit128 { u32 w[4]; void ClrAll(); };
void EXBit128::ClrAll() { w[0] = 0; w[1] = 0; w[2] = 0; w[3] = 0; }
}
