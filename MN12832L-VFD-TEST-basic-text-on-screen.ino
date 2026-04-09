// ============================================================
//  MN12832L VFD Driver - CLEAN BASIC TEST (ORIGINAL PIXELS)
//  Target: ESP32 WROOM-32D / ESP32-S3
// ============================================================

#include <SPI.h>

// --- Pin Definitions (Sadece 4 Ana Pin) ---
#define PIN_VFD_MOSI   23  // SIN
#define PIN_VFD_SCK    18  // CLK
#define PIN_VFD_LAT    22  // LAT
#define PIN_VFD_BLK    21  // BK

#define VFD_SPI_FREQ  2000000UL  
#define MIRROR_X true 
#define MIRROR_Y false
#define VFD_GRIDS    43    
#define VFD_BYTES    30

static uint8_t framebuffer[32][16]; 

// ============================================================
//  MİLAT KODU - DUAL GRID (HİÇBİR PİKSELE DOKUNULMADI)
// ============================================================
void build_frame(uint8_t* buf, int grid) {
    memset(buf, 0, VFD_BYTES);
    int col_start = grid * 3; 
    
    int offsets[3];
    if (grid % 2 == 0) {
        offsets[0] = 0; offsets[1] = 2; offsets[2] = 4;
    } else {
        offsets[0] = 5; offsets[1] = 3; offsets[2] = 1;
    }

    for (int row = 0; row < 32; row++) {
        for (int i = 0; i < 3; i++) {
            int fb_col = col_start + i;
            if (fb_col >= 128) continue; 
            
            int read_x = MIRROR_X ? (127 - fb_col) : fb_col;
            int read_y = MIRROR_Y ? (31 - row) : row;

            bool pixel_on = (framebuffer[read_y][read_x / 8] & (0x80 >> (read_x % 8))) != 0;
            
            if (pixel_on) {
                int bit_pos = (row * 6) + offsets[i];  
                buf[bit_pos / 8] |= (0x80 >> (bit_pos % 8));
            }
        }
    }

    // Orijinal Dual Grid Mantığı (Next Grid) - Parlaklık için
    int grid_bit_pos = 192 + grid; 
    buf[grid_bit_pos / 8] |= (0x80 >> (grid_bit_pos % 8));
    
    int next_grid = 192 + grid + 1;
    buf[next_grid / 8] |= (0x80 >> (next_grid % 8));
}

void vfd_latch() {
    digitalWrite(PIN_VFD_LAT, HIGH);
    delayMicroseconds(1);  
    digitalWrite(PIN_VFD_LAT, LOW);
}

void vfd_scan() {
    uint8_t frame[VFD_BYTES];
    SPISettings settings(VFD_SPI_FREQ, MSBFIRST, SPI_MODE0); 
    
    for (int g = 0; g < VFD_GRIDS; g++) {
        build_frame(frame, g);
        SPI.beginTransaction(settings);
        SPI.writeBytes(frame, VFD_BYTES);
        SPI.endTransaction();
        
        digitalWrite(PIN_VFD_BLK, HIGH); 
        vfd_latch();
        delayMicroseconds(5); 
        digitalWrite(PIN_VFD_BLK, LOW);  
        delayMicroseconds(60); 
    }
}

// ============================================================
//  Çizim Araçları
// ============================================================
void fb_clear() { memset(framebuffer, 0, sizeof(framebuffer)); }

void fb_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 32) return;
    if (on) framebuffer[y][x / 8] |= (0x80 >> (x % 8));
    else    framebuffer[y][x / 8] &= ~(0x80 >> (x % 8));
}

void fb_hline(int x, int y, int w, bool on) {
    for (int dx = 0; dx < w; dx++) fb_set_pixel(x + dx, y, on);
}

void fb_vline(int x, int y, int h, bool on) {
    for (int dy = 0; dy < h; dy++) fb_set_pixel(x, y + dy, on);
}

void fb_frame() {
    fb_hline(0, 0, 128, true);  fb_hline(0, 31, 128, true);  
    fb_vline(0, 0, 32, true);   fb_vline(127, 0, 32, true);  
}

const uint8_t FONT5x7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, 
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10}, 
    {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, 
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x7E,0x11,0x11,0x11,0x7E}, 
    {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C}, 
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, 
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, 
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, 
    {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06}, 
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, 
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, 
    {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, 
    {0x61,0x51,0x49,0x45,0x43}, 
};

void fb_char(int x, int y, char c) {
    int idx = -1;
    if (c == ' ') idx = 0;
    else if (c >= '0' && c <= '9') idx = 1 + (c - '0');
    else if (c >= 'A' && c <= 'Z') idx = 11 + (c - 'A');
    else if (c >= 'a' && c <= 'z') idx = 11 + (c - 'a'); 
    if (idx < 0) return;
    for (int col = 0; col < 5; col++) {
        uint8_t col_data = pgm_read_byte(&FONT5x7[idx][col]);
        for (int row = 0; row < 7; row++) {
            fb_set_pixel(x + col, y + row, (col_data >> (6 - row)) & 1);
        }
    }
}

void fb_str(int x, int y, const char* s) { while (*s) { fb_char(x, y, *s++); x += 6; } }

// ============================================================
void setup() {
    Serial.begin(115200);
    pinMode(PIN_VFD_LAT, OUTPUT);
    pinMode(PIN_VFD_BLK, OUTPUT);
    
    digitalWrite(PIN_VFD_BLK, LOW);   
    digitalWrite(PIN_VFD_LAT, LOW);   
    
    SPI.begin(PIN_VFD_SCK, -1, PIN_VFD_MOSI, -1);
    delay(100); 
    
    fb_clear();
    fb_frame();
    fb_str(36, 12, "VFD TEST"); 
}

void loop() {
    vfd_scan();
}
