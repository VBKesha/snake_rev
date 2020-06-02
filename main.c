#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t map[4096];
uint8_t high_map[256];
uint8_t block_type[256];

uint8_t bonus_offset[6];
uint8_t bonuses[84][2];

uint8_t bonus_levels[1024];
uint16_t bl_offsets[4] = {
    0x0000,
    0x022A - 0x01DA,
    0x029E - 0x01DA,
    0x030E - 0x01DA,
};
uint8_t uw_levels[1024];
uint16_t uw_offsets[5] = {
    0x0000,
    0x06FE - 0x0678,
    0x0774 - 0x0678,
    0x07DC - 0x0678,
    0x07DC - 0x0678,
};

uint16_t items_offsets[18] = {
    0x08F6,
    0x096D, 0x09EB, 0x0A93, 0x0B2D, 0x0BCE, 0x0C76, 0x0D09,
    0x0D17, 0x0DB1, 0x0E21, 0x0E59, 0x0E8A, 0x0EC9,
    0x0F08, 0x0F4E, 0x0F78,
    0x0F78 + 77};

uint8_t items_by_level[17][2] = {
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
    {8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5},
    {9, 0}, {10, 0}, {11, 0},
    {7, 1},
};

uint8_t items[249][7];

void read_world();
void genBox(FILE * max_out, uint8_t x, uint8_t y, uint16_t high, uint8_t type);
void genText(FILE * max_out, uint8_t x, uint8_t y, uint16_t high, uint8_t type);
uint8_t getHigh(uint8_t x, uint8_t y);
uint8_t getBlockType(uint8_t x, uint8_t y);
void bonuses_dec();
void underwater_dec();
void decode_items();

#define LEVEL9_UP (114)

FILE * max_out;

int main(){
    uint32_t i;
    uint16_t tmp;

    read_world();

    max_out = fopen("level.ms", "w+");
    for(uint8_t y=0; y<64; y++){
        for(uint8_t x=0; x<64; x++){
                genBox(max_out, x, y, getHigh(x,y), getBlockType(x, y));
                genText(max_out,x, y, getHigh(x,y), getBlockType(x, y));
        }
    }
    fclose(max_out);

    bonuses_dec();
    underwater_dec();


    for(uint8_t y=0; y<64; y++){
        for(uint8_t x=0; x<64; x++){
            if(getBlockType(x, y) == 2){
                printf("BN[%02u:%02u] = [ ", x, y);
                for(uint8_t n=0; n<84; n++){
                    uint8_t bx, by, bt;
                    bx = ((bonuses[n][0] >> 4)& 0x0F) | ((bonuses[n][0] << 4) & 0xF0);
                    by = ((bonuses[n][1] >> 4)& 0x0F);
                    bt = bonuses[n][1] & 0x0F;
                    //printf("[%02X:%02X]\r\n", bonuses[n][0], bonuses[n][1]);
                    //printf("[%03u, %03u][%x]\r\n", bx, by, bt);
                    if((bx == x) && ((y & 0x0F) == by)){
                        printf("%X ", bt);
                    }
                }
                printf("]\r\n");
            }
        }
    }

    decode_items();

    return 0;
}

uint8_t getHigh(uint8_t x, uint8_t y){
    return high_map[map[y*64 + x]];
}


uint8_t getBlockType(uint8_t x, uint8_t y){
    uint8_t block_id;
    uint8_t ret;
    uint8_t level_id;
    level_id = 0;
    if((x<29) && (y>35)){
        level_id = 0x80;
    }

    block_id = map[y*64 + x];
    ret = block_type[(block_id >> 1) | level_id];

    if((block_id & 0x01) == 1) {
        ret = ret >> 4;
    }
    ret &= 0x0F;

    return ret;
}

void genText(FILE * max_out, uint8_t x, uint8_t y, uint16_t high, uint8_t type){
    float fy;
    fy = y*16 - 1.5;
    if((x<29) && (y>35)){
        high += LEVEL9_UP;
    }

    fprintf(max_out, "text size:5 font:\"Courier New\" text:\"%X\" pos:[%d,%03.01f,%d.1] wirecolor:(color 108 8 136) name:\"TX[%02d:%02d]\" \r\n", type, x*16, fy, high*16, x,y);
}

void genSphere(FILE * max_out, uint16_t x, uint16_t y, uint16_t z, uint8_t type){
        fprintf(max_out, "Sphere radius:8 smooth:on segs:16 chop:0 slice:off sliceFrom:0 sliceTo:0 mapCoords:off recenter:off pos:[%d,%d,%d] wirecolor:(color 255 00 00) \
           name:\"SP[%02X]\"\r\n",
           x, y, z, type);
}

void genCyl(FILE * max_out, uint16_t x, uint16_t y, uint16_t z, uint8_t type){
        fprintf(max_out, "Cylinder smooth:on heightsegs:1 capsegs:1 sides:18 height:2 radius:5 mapCoords:off recenter:off pos:[%d,%d,%d] wirecolor:(color 255 255 00) \
           name:\"WRG[%02X]\"\r\n",
           x, y, z, type);
}


void genBox(FILE * max_out, uint8_t x, uint8_t y, uint16_t high, uint8_t type){
    uint8_t color;
    uint8_t color_map[2][2] = {{1,0}, {0,1}};
    if((x<29) && (y>35)){
        high += LEVEL9_UP;
    }

    fprintf(max_out, "Box lengthsegs:1 widthsegs:1 heightsegs:1 length:16 width:16 height:%d mapCoords:off pos:[%d,%d,0] name:\"Box[%02d:%02d][BT%02X]\" ", high*16, x*16, y*16, x ,y, type);
    color = color_map[(x % 2)][(y % 2)];

    if((high > 0) && (high < 114)){
            if(type != 0xA){
                if(color == 1){
                    fprintf(max_out, "wirecolor:(color 00 200 00)");
                } else {
                    fprintf(max_out, "wirecolor:(color 00 150 00)");
                }
            } else {
                fprintf(max_out, "wirecolor:(color 00 00 230)");
            }
    } else if (high >= 114){
        fprintf(max_out, "wirecolor:(color 200 200 250)");
    } else {
        fprintf(max_out, "wirecolor:(color 00 00 255)");
    }

    fprintf(max_out, "\r\n");

}

void decode_items(){
    FILE * file;
    FILE * file_ms;
    char fname[32];
    uint8_t i, j, len, offset;
    uint8_t lvl, sub_lvl;
    sprintf(fname, "items_dec.csv");
    file = fopen(fname, "w+");
    fprintf(file, "lvl;sub_lvl;ID;X;Y;Z;PARAMS;\n");
    for(i=0; i < (sizeof(items_offsets) / 2) - 1 ; i++){
        if(i<11){
            sprintf(fname, "items\\items_%02d.ms", i+1);
        } else {
            sprintf(fname, "items\\uw_items_%02d.ms", i-10);
        }
        file_ms = fopen(fname, "w+");
        len = (items_offsets[i+1] - items_offsets[i]) / 7;
        offset = (items_offsets[i] - items_offsets[0]) / 7;
        for(j=0; j<len; j++){
            uint16_t x = items[offset+j][2] + (256 * ((items[offset+j][1] & 0xF0) >> 5)) - 8;
            uint16_t y = items[offset+j][3] + (256 * ((items[offset+j][1] & 0x0F) >> 2)) - 8;
            uint16_t z = items[offset+j][4] + (256 * (((items[offset+j][1] << 1) | ((items[offset+j][5]) >> 7))  & 0x7)) + 8;

            if(i<11){
                lvl = i+1;
                sub_lvl = 0;
            } else {
                lvl = 8;
                sub_lvl = i - 10;
            }
            if(i==16){
                lvl = 7;
                sub_lvl = 1;
            }
            fprintf(file, "%u;%u;0x%02X;%u;%u;%u;0x%04X;\n", lvl, sub_lvl, items[offset+j][0], x,y,z, (items[offset+j][5] & 0x7F) << 8 | (items[offset+j][6]));
        }
        fclose(file_ms);
    }
    fclose(file);
}

void bonuses_dec(){
    uint32_t cnt, offset, i, j;
    uint8_t item, count, bnn;
    uint8_t x, y, block_id;
    FILE * file;
    char fname[32];
    for(i=0; i<4; i++){
        printf("Decode bonus level %d\r\n", i+1);
        sprintf(fname, "bonus_level_%d.ms", i+1);
        file = fopen(fname, "w+");
        offset = bl_offsets[i];
        cnt = 0;
        x = 0; y = 0;
        do {
            item = bonus_levels[offset++];
            count = bonus_levels[offset++];
            for(j = 0; j < count; j++){
                cnt++;

                block_id = block_type[(item >> 1)];
                if((item & 0x01) == 1) {
                    block_id = block_id >> 4;
                }
                block_id &= 0x0F;

                genBox(file, x, y, high_map[item], block_id);
                genText(file,x, y, high_map[item], block_id);

                x++;
                if(x > 15){ y++; x = 0;}
            }
        } while (cnt<256);
        fclose(file);
    }
}

void underwater_dec(){
    uint32_t cnt, offset, i, j;
    uint8_t item, count, bnn;
    uint8_t x, y, block_id;
    FILE * file;
    char fname[32];
    for(i=0; i<5; i++){
        printf("Decode underwater level %d\r\n", i+1);
        sprintf(fname, "underwater_level_%d.ms", i+1);
        file = fopen(fname, "w+");
        offset = uw_offsets[i];
        cnt = 0;
        x = 0; y = 0;
        do {
            item = uw_levels[offset++];
            count = uw_levels[offset++];
            for(j = 0; j < count; j++){
                cnt++;

                block_id = block_type[(item >> 1)];
                if((item & 0x01) == 1) {
                    block_id = block_id >> 4;
                }
                block_id &= 0x0F;

                genBox(file, x, y, high_map[item], block_id);
                genText(file,x, y, high_map[item], block_id);

                x++;
                if(x > 15){ y++; x = 0;}
            }
        } while (cnt<256);
        fclose(file);
    }
}

void read_world(){
    uint32_t readed;
    FILE * file;

    file = fopen("Snake_Rattle'n_Roll_(U).nes", "rb");

    fseek(file, 0x63D0, SEEK_SET);
    readed = fread(map, 4096, 1, file);
    printf("Map Readed: %d\r\n", readed);

    fseek(file, 0x5079, SEEK_SET);
    readed = fread(high_map, 256, 1, file);
    printf("HighMap Readed: %d\r\n", readed);


    fseek(file, 0x4F7A, SEEK_SET);
    readed = fread(block_type, 256, 1, file);
    printf("Block_type Readed: %d\r\n", readed);

    fseek(file, 0xF4D0, SEEK_SET);
    readed = fread(bonus_offset, 6, 1, file);
    printf("Bonuses offsets: %d\r\n", readed);

    fseek(file, 0xF4D0+6, SEEK_SET);
    readed = fread(bonuses, 168, 1, file);
    printf("Level Readed: %d\r\n", readed);


    fseek(file, 0xE1EA, SEEK_SET);
    readed = fread(bonus_levels, 1024, 1, file);
    printf("Bonus levels Readed: %d\r\n", readed);

    fseek(file, 0xE688, SEEK_SET);
    readed = fread(uw_levels, 1024, 1, file);
    printf("Underwater levels Readed: %d\r\n", readed);

    fseek(file, 0xE906, SEEK_SET);
    readed = fread(items, items_offsets[17] - items_offsets[0], 1, file);
    printf("Items on levels Readed: %d\r\n", readed);

    fclose(file);
}
