// ============================================================
//  MN12832L VFD Driver - ZOGOLDER ANIMATION (ADAM GİBİ FİNAL)
//  Target: ESP32 WROOM-32D / ESP32-S3
// ============================================================

#include <SPI.h>

// --- Pin Definitions (Sadece Gerekli Olanlar) ---
#define PIN_VFD_MOSI   23  
#define PIN_VFD_SCK    18  
#define PIN_VFD_LAT    22  
#define PIN_VFD_BLK    21  

#define VFD_SPI_FREQ  2000000UL  
#define MIRROR_X true 
#define MIRROR_Y false
#define VFD_GRIDS    43    
#define VFD_BYTES    30

static uint8_t framebuffer[32][16]; 
static uint8_t text_mask[32][16]; 
bool apply_mask = false; 

unsigned long last_anim_time = 0;
int anim_state = 0;
int fill_level[128]; 
int drop_y[128];     
int line_width = 0;      
float slide_progress = 0; 

// ============================================================
//  MİLAT KODU (DOKUNULMAZ ALTYAPI)
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

    // ============================================================
    // İŞTE BÜTÜN SORUNUN ÇÖZÜMÜ BURASI (1. SÜTUN FIX)
    // Benim yazdığım o saçma 'if'leri çöpe attık. 
    // Dümdüz senin çalışan orijinal mantığın:
    // ============================================================
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

void fb_clear() { memset(framebuffer, 0, sizeof(framebuffer)); }
void set_mask_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 32) return;
    if (on) text_mask[y][x / 8] |= (0x80 >> (x % 8));
    else    text_mask[y][x / 8] &= ~(0x80 >> (x % 8));
}
bool get_mask_pixel(int x, int y) {
    if (x < 0 || x >= 128 || y < 0 || y >= 32) return false;
    return (text_mask[y][x / 8] & (0x80 >> (x % 8))) != 0;
}
void fb_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 32) return;
    if (apply_mask && on && get_mask_pixel(x, y)) return; 
    if (on) framebuffer[y][x / 8] |= (0x80 >> (x % 8));
    else    framebuffer[y][x / 8] &= ~(0x80 >> (x % 8));
}
void fb_hline(int x, int y, int w, bool on) {
    for (int dx = 0; dx < w; dx++) fb_set_pixel(x + dx, y, on);
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

void draw_italic_scaled_char(int x, int y, char c, int scale) {
    int idx = -1;
    if (c >= 'A' && c <= 'Z') idx = 11 + (c - 'A');
    else if (c == ' ') idx = 0;
    if (idx < 0) return;
    for (int col = 0; col < 5; col++) {
        uint8_t col_data = pgm_read_byte(&FONT5x7[idx][col]);
        for (int row = 0; row < 7; row++) {
            if ((col_data >> (6 - row)) & 1) {
                for(int px=0; px<scale; px++) {
                    for(int py=0; py<scale; py++) {
                        int local_y = (row * scale) + py;
                        int slant = (local_y - (7 * scale)) / 2; 
                        set_mask_pixel(x + (col * scale) + px + slant, y + local_y, true);
                    }
                }
            }
        }
    }
}

void draw_italic_scaled_str(int x, int y, const char* s, int scale) {
    while (*s) { draw_italic_scaled_char(x, y, *s++, scale); x += (5 * scale) + scale; }
}

void fb_str(int x, int y, const char* s) {
    while (*s) {
        int idx = -1;
        if (*s >= 'A' && *s <= 'Z') idx = 11 + (*s - 'A');
        else if (*s >= 'a' && *s <= 'z') idx = 11 + (*s - 'a');
        else if (*s >= '0' && *s <= '9') idx = 1 + (*s - '0');
        else if (*s == 'x') idx = 34; 
        else if (*s == ' ') idx = 0;
        if (idx >= 0) {
            for (int col = 0; col < 5; col++) {
                uint8_t col_data = pgm_read_byte(&FONT5x7[idx][col]);
                for (int row = 0; row < 7; row++) {
                    if ((col_data >> (6 - row)) & 1) fb_set_pixel(x + col, y + row, true);
                }
            }
        }
        x += 6; s++;
    }
}

// ============================================================
//  MAIN ARDUINO SETUP & LOOP
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
    randomSeed(analogRead(0));
}

void loop() {
    unsigned long current_time = millis();
    
    switch(anim_state) {
        case 0: // HAZIRLIK
            fb_clear(); 
            memset(text_mask, 0, sizeof(text_mask)); 
            draw_italic_scaled_str(21, 9, "ZOGOLDER", 2); 
            apply_mask = true; 
            for(int i = 0; i < 128; i++) { 
                fill_level[i] = 32; 
                drop_y[i] = -random(0, 50); 
            }
            line_width = 0; slide_progress = 0; anim_state = 1;
            break;

        case 1: // YAĞMUR ANİMASYONU
            if (current_time - last_anim_time > 15) { 
                last_anim_time = current_time;
                bool is_screen_full = true;
                for(int i = 0; i < 128; i++) {
                    if (fill_level[i] > 0) { 
                        is_screen_full = false;
                        if (drop_y[i] >= 0 && drop_y[i] < fill_level[i]) fb_set_pixel(i, drop_y[i], false); 
                        drop_y[i]++; 
                        if (drop_y[i] >= fill_level[i] - 1) { 
                            fb_set_pixel(i, fill_level[i] - 1, true); 
                            fill_level[i]--; 
                            drop_y[i] = -random(2, 25); 
                        } 
                        else if (drop_y[i] >= 0) fb_set_pixel(i, drop_y[i], true); 
                    }
                }
                if (is_screen_full) anim_state = 2;
            }
            break;

        case 2: // BEKLEME
            if (current_time - last_anim_time > 3000) { anim_state = 3; last_anim_time = current_time; } 
            break;

        case 3: // YAZIYA GEÇİŞ
            apply_mask = false; fb_clear(); fb_str(34, 16, "128x32 VFD"); 
            anim_state = 4; last_anim_time = current_time; 
            break;

        case 4: // BEKLEME
            if (current_time - last_anim_time > 1500) { anim_state = 5; last_anim_time = current_time; } 
            break;

        case 5: // LAZER ÇİZGİ AÇILIŞI
            if (current_time - last_anim_time > 15) { 
                last_anim_time = current_time; line_width += 3;
                fb_clear(); fb_str(34, 16, "128x32 VFD"); 
                fb_hline(64 - line_width, 16, line_width * 2, true);
                if (line_width >= 50) anim_state = 6;
            }
            break;

        case 6: // ASANSÖR EFEKTİ
            if (current_time - last_anim_time > 40) { 
                last_anim_time = current_time; slide_progress += 1.0;
                fb_clear();
                fb_str(34, 14 + (slide_progress * 1.0), "128x32 VFD"); 
                fb_hline(64 - line_width, 16, line_width * 2, true); 
                fb_str(34, 16 - (slide_progress * 1.6), "with ESP32"); 
                
                if (slide_progress >= 8) { anim_state = 7; last_anim_time = current_time; }
            }
            break;

        case 7: // BİTİŞ VE BAŞA SAR
            if (current_time - last_anim_time > 5000) anim_state = 0; 
            break;
    }
    
    // EKRAN TARAMA (SÜREKLİ ÇALIŞMALI)
    vfd_scan(); 
}