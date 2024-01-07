
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

#define ICO_HEADER_SZ 22
#define BMP_HEADER_SZ 40
#define COLOR_TBL_SZ 64 /* 16 * 4-byte colors */
#define ICO_DATA_SZ 192 /* (h * (w / 2)) + 64-bit AND mask */

#define ICO_TRANSPARENT_COLOR 13

#define BMPINFO_OFFSET_WIDTH 4
#define BMPINFO_OFFSET_HEIGHT 8
#define BMPINFO_OFFSET_BPP 14
#define BMPINFO_OFFSET_CMP 16
#define BMPINFO_OFFSET_SZ 20
#define BMPINFO_OFFSET_PAL_SZ 32

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

#endif /* !ICOTOOLS_H */

