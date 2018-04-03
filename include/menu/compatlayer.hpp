#if TEXTMODE_VAC != 1
namespace colorsint
{
constexpr int Create(int r, int g, int b, int a)
{
    unsigned char _r = (r) &0xFF;
    unsigned char _g = (g) &0xFF;
    unsigned char _b = (b) &0xFF;
    unsigned char _a = (a) &0xFF;
    return (int) (_r) | (int) (_g << 8) | (int) (_b << 16) | (int) (_a << 24);
}
constexpr int Transparent(int base, float mod = 0.5f)
{
    unsigned char _a = (base >> 24) & 0xFF;
    unsigned char _b = (base >> 16) & 0xFF;
    unsigned char _g = (base >> 8) & 0xFF;
    unsigned char _r = (base) &0xFF;
    return Create(_r, _g, _b, (int) ((float) (_a) *mod));
}
int FromHSL(float h, float s, float l);
int RainbowCurrent();
constexpr int pink = Create(255, 105, 180, 255);

constexpr int white = Create(255, 255, 255, 255);
constexpr int black = Create(0, 0, 0, 255);

constexpr int red = Create(237, 42, 42, 255), blu = Create(28, 108, 237, 255);
constexpr int red_b  = Create(64, 32, 32, 178),
              blu_b  = Create(32, 32, 64, 178); // Background
constexpr int red_v  = Create(196, 102, 108, 255),
              blu_v  = Create(102, 182, 196, 255); // Vaccinator
constexpr int red_u  = Create(216, 34, 186, 255),
              blu_u  = Create(167, 75, 252, 255); // Ubercharged
constexpr int yellow = Create(255, 255, 0, 255);
constexpr int orange = Create(255, 120, 0, 255);
constexpr int green  = Create(0, 255, 0, 255);
}

namespace draw
{
void String(unsigned long font, int x, int y, int color, int shadow,
            const char *text);
void String(unsigned long font, int x, int y, int color, int shadow,
            std::string text);
void WString(unsigned long font, int x, int y, int color, int shadow,
             const wchar_t *text);
void FString(unsigned long font, int x, int y, int color, int shadow,
             const char *text, ...);

void DrawRect(int x, int y, int w, int h, int color);
void DrawLine(int x, int y, int dx, int dy, int color);
void OutlineRect(int x, int y, int w, int h, int color);
void GetStringLength(unsigned long font, char *string, int &length,
                     int &height);
std::pair<int, int> GetStringLength(unsigned long font, std::string string);
}
#endif
