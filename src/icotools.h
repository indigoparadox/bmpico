
#ifndef ICOTOOLS_H
#define ICOTOOLS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ICOTOOLS_ERR_FILE 2
#define ICOTOOLS_ERR_ALLOC 4
#define ICOTOOLS_ERR_READ 8
#define ICOTOOLS_ERR_VERIFY 16

#define ICO_FILE_HEADER_SZ 6
#define ICO_ENTRY_HEADER_SZ 16
#define BMP_HEADER_SZ 40

#define BMPINFO_OFFSET_WIDTH 4
#define BMPINFO_OFFSET_HEIGHT 8
#define BMPINFO_OFFSET_COLOR_PLANES 12
#define BMPINFO_OFFSET_BPP 14
#define BMPINFO_OFFSET_CMP 16
#define BMPINFO_OFFSET_SZ 20
#define BMPINFO_OFFSET_HRES 24
#define BMPINFO_OFFSET_VRES 28
#define BMPINFO_OFFSET_PAL_SZ 32
#define BMPINFO_OFFSET_IMP_COLORS 36

#define ICOENTRY_OFFSET_WIDTH 0
#define ICOENTRY_OFFSET_HEIGHT 1
#define ICOENTRY_OFFSET_PAL_SZ 2
#define ICOENTRY_OFFSET_COLOR_PLANES 4
#define ICOENTRY_OFFSET_BPP 6
#define ICOENTRY_OFFSET_BMP_SZ 8
#define ICOENTRY_OFFSET_BMP_OFFSET 12

#define ICOTOOLS_FLAG_VERIFY_BPP 1
#define ICOTOOLS_FLAG_VERIFY_DIMX 2

struct ICOTOOLS_BMP_INFO {
   uint32_t header_sz;
   int32_t w_px;
   int32_t h_px;
   uint16_t color_planes;
   uint16_t bpp;
   uint32_t compression;
   uint32_t image_sz;
   int32_t ppm_h;
   int32_t ppm_v;
   uint32_t pal_sz;
   uint32_t important_c;
};

struct ICOTOOLS_ICO_ENTRY {
   uint8_t w_px;
   uint8_t h_px;
   uint8_t pal_sz;
   uint8_t res;
   uint16_t color_planes;
   uint16_t bpp;
   uint32_t bmp_sz;
   uint32_t bmp_offset;
};

#define icotools_bprintf( num ) \
   printf( "%u", ((num) >> 7) & 0x01 ); \
   printf( "%u", ((num) >> 6) & 0x01 ); \
   printf( "%u", ((num) >> 5) & 0x01 ); \
   printf( "%u", ((num) >> 4) & 0x01 ); \
   printf( "%u", ((num) >> 3) & 0x01 ); \
   printf( "%u", ((num) >> 2) & 0x01 ); \
   printf( "%u", ((num) >> 1) & 0x01 ); \
   printf( "%u", (num) & 0x01 );

#define icotools_read_u16( bytes, offset ) \
   ((bytes[offset]) | (bytes[offset + 1] << 8))

#define icotools_read_u32( bytes, offset ) \
   ((bytes[offset]) | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) \
   | (bytes[offset + 3] << 24))

#define icotools_write_u8( file, num, buf8 ) \
   assert( sizeof( buf8 ) == 1 ); \
   buf8 = (num) & 0xff; \
   fwrite( &buf8, 1, 1, file );

#define icotools_write_u16( file, num, buf8 ) \
   assert( sizeof( buf8 ) == 1 ); \
   buf8 = (num) & 0xff; \
   fwrite( &buf8, 1, 1, file ); \
   buf8 = ((num) >> 8) & 0xff; \
   fwrite( &buf8, 1, 1, file );

#define icotools_write_u32( file, num, buf8 ) \
   assert( sizeof( buf8 ) == 1 ); \
   buf8 = (num) & 0xff; \
   fwrite( &buf8, 1, 1, file ); \
   buf8 = ((num) >> 8) & 0xff; \
   fwrite( &buf8, 1, 1, file ); \
   buf8 = ((num) >> 16) & 0xff; \
   fwrite( &buf8, 1, 1, file ); \
   buf8 = ((num) >> 24) & 0xff; \
   fwrite( &buf8, 1, 1, file );

#define icotools_xor_mask_sz( w_px, h_px ) (((w_px) / 2) * (h_px))

#define icotools_and_mask_sz( h_px ) (4 * (h_px))

int icotools_read_file(
   const char* filename, uint8_t** buffer, size_t* sz, size_t offset
) {
   int retval = 0;
   FILE* in_file = NULL;
   size_t bytes_read = 0;

   /* Open file and allocate buffer. */

   in_file = fopen( filename, "rb" );
   if( NULL == in_file ) {
      retval = ICOTOOLS_ERR_FILE;
      goto cleanup;
   }

   fseek( in_file, 0, SEEK_END );
   *sz = ftell( in_file ) - offset;
   fseek( in_file, offset, SEEK_SET );

   *buffer = calloc( *sz, 1 );
   if( NULL == *buffer ) {
      retval = ICOTOOLS_ERR_ALLOC;
      goto cleanup;
   }
   
   bytes_read = fread( *buffer, 1, *sz, in_file );
   if( bytes_read < *sz ) {
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

cleanup:

   if( NULL != in_file ) {
      fclose( in_file );
   }

   return retval;
}

int icotools_verify_bmp(
   uint8_t* bmp_bytes, struct ICOTOOLS_BMP_INFO* bmp, uint8_t flags
) {
   int retval = 0;

   bmp->header_sz = icotools_read_u16( bmp_bytes, 0 );
   if( 40 != bmp->header_sz ) {
      fprintf( stderr, "invalid bitmap header!\n" );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   bmp->bpp = icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_BPP );
   if(
      ICOTOOLS_FLAG_VERIFY_BPP == (ICOTOOLS_FLAG_VERIFY_BPP & flags) &&
      8 != bmp->bpp
   ) {
      fprintf( stderr, "invalid bitmap bpp: %u\n", bmp->bpp );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   bmp->w_px = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_WIDTH );
   if(
      ICOTOOLS_FLAG_VERIFY_DIMX == (ICOTOOLS_FLAG_VERIFY_DIMX & flags) &&
      16 != bmp->w_px && 32 != bmp->w_px
   ) {
      fprintf( stderr, "invalid bitmap width: %u\n", bmp->w_px );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   bmp->h_px = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_HEIGHT );
   if(
      ICOTOOLS_FLAG_VERIFY_DIMX == (ICOTOOLS_FLAG_VERIFY_DIMX & flags) &&
      16 != bmp->h_px && 32 != bmp->h_px
   ) {
      fprintf( stderr, "invalid bitmap height: %u\n", bmp->h_px );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   if(
      ICOTOOLS_FLAG_VERIFY_DIMX == (ICOTOOLS_FLAG_VERIFY_DIMX & flags) &&
      bmp->w_px != bmp->h_px 
   ) {
      fprintf( stderr, "bitmap width/height mismatch!\n" );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   bmp->compression = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_CMP );
   if( 0 != bmp->compression ) {
      fprintf( stderr,
         "invalid bitmap compression: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_CMP ) );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   bmp->color_planes =
      icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_COLOR_PLANES );
   bmp->image_sz =
      icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_SZ );
   bmp->important_c =
      icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_IMP_COLORS );
   bmp->ppm_h = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_HRES );
   bmp->ppm_v = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_VRES );

   bmp->pal_sz = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_PAL_SZ );
   if( 0 == bmp->pal_sz ) {
      fprintf( stderr, "invalid bitmap palette size: %u\n", bmp->pal_sz );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

cleanup:
   return retval;
}

int icotools_verify_ico_entry(
   uint8_t* icoentry_bytes, struct ICOTOOLS_ICO_ENTRY* ico_entry
) {
   ico_entry->w_px = icoentry_bytes[ICOENTRY_OFFSET_WIDTH];
   ico_entry->h_px = icoentry_bytes[ICOENTRY_OFFSET_HEIGHT];
   ico_entry->pal_sz = icoentry_bytes[ICOENTRY_OFFSET_PAL_SZ];
   ico_entry->color_planes =
      icotools_read_u16( icoentry_bytes, ICOENTRY_OFFSET_COLOR_PLANES );
   ico_entry->bpp =
      icotools_read_u16( icoentry_bytes, ICOENTRY_OFFSET_BPP );
   ico_entry->bmp_offset =
      icotools_read_u32( icoentry_bytes, ICOENTRY_OFFSET_BMP_OFFSET );
   ico_entry->bmp_sz = 
      icotools_read_u32( icoentry_bytes, ICOENTRY_OFFSET_BMP_SZ );
   return 0;
}

#endif /* !ICOTOOLS_H */

