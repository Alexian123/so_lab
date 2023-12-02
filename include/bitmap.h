#ifndef BITMAP_H
#define BITMAP_H

    #include <stdint.h>

    struct BMPHeader 
    {
        uint16_t signature;
        uint32_t fileSize;
        uint32_t reserved;
        uint32_t dataOffset;
    } __attribute__((packed));

    struct BMPInfoHeader
    {
        uint32_t size;
        uint32_t width;
        uint32_t height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t imageSize;
        uint32_t xPixelsPerM;
        uint32_t yPixelsPerM;
        uint32_t colorsUsed;
        uint32_t colorsImportant;
    } __attribute__((packed));

    struct BMPFile
    {
        struct BMPHeader header;
        struct BMPInfoHeader info;
        uint8_t *data;
    };

#endif