
#include "icotools.h"

int main( int argc, char* argv[] ) {
   int retval = 0;
   uint8_t* ico_bytes = NULL;
   uint8_t* ico_px = NULL;
   uint16_t ico_field_res = 0;
   uint16_t ico_field_type = 0;
   uint16_t ico_field_num_imgs = 0;
   uint32_t ico_img_offset = 0;
   uint32_t ico_img_sz = 0;
   size_t ico_sz = 0;
   size_t ico_bytes_read = 0;
   size_t i = 0;
   uint8_t px_byte = 0;
   
   if( 2 > argc ) {
      fprintf( stderr, "usage: %s <ico_file>\n", argv[0] );
      retval = 1;
      goto cleanup;
   }

   retval = icotools_read_file( argv[1], &ico_bytes, &ico_sz, 0 );
   if( 0 != retval ) {
      goto cleanup;
   }

   /* Sanity check: Fields */

   ico_field_res = icotools_read_u16( ico_bytes, 0 );
   if( 0 != ico_field_res ) {
      fprintf( stderr, "invalid: field reserved: %u\n", ico_field_res );
      retval = 16;
      goto cleanup;
   }

   ico_field_type = icotools_read_u16( ico_bytes, 2 );
   if( 1 != ico_field_type ) {
      fprintf( stderr, "invalid: field type: %u\n", ico_field_type );
      retval = 16;
      goto cleanup;
   }

   ico_field_num_imgs = icotools_read_u16( ico_bytes, 4 );
   if( 1 != ico_field_num_imgs ) {
      fprintf( stderr, "invalid: field num_imgs: %u\n", ico_field_num_imgs );
      retval = 16;
      goto cleanup;
   }

   if( 16 != ico_bytes[6] ) { 
      fprintf( stderr, "invalid: field ico_width: %u\n", ico_bytes[6] );
      retval = 16;
      goto cleanup;
   }

   if( 16 != ico_bytes[7] ) { 
      fprintf( stderr, "invalid: field ico_height: %u\n", ico_bytes[7] );
      retval = 16;
      goto cleanup;
   }

   if( 16 != ico_bytes[8] ) { 
      fprintf( stderr, "invalid: field ico_colors: %u\n", ico_bytes[8] );
      retval = 16;
      goto cleanup;
   }

   ico_img_sz = icotools_read_u32( ico_bytes, 14 );
   ico_img_offset = icotools_read_u32( ico_bytes, 18 );
   printf(
      "ico data at %u bytes, %u bytes long\n", ico_img_offset, ico_img_sz );

   /* Print bitmap header info. */
   printf( "bitmap width: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 4 ) );
   printf( "bitmap height: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 8 ) );
   printf( "bitmap color planes: %u\n", icotools_read_u16( ico_bytes,
      ICO_HEADER_SZ + 12 ) );
   printf( "bitmap bpp: %u\n", icotools_read_u16( ico_bytes,
      ICO_HEADER_SZ + 14 ) );
   printf( "bitmap compression: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 16 ) );
   printf( "bitmap size: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 20 ) );
   printf( "bitmap hres: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 24 ) );
   printf( "bitmap vres: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 28 ) );
   printf( "bitmap colors: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 32 ) );
   printf( "bitmap imp colors: %u\n", icotools_read_u32( ico_bytes,
      ICO_HEADER_SZ + 36 ) );

   /* Print color palette. */
   for( i = 0 ; 16 > i ; i++ ) {
      printf( "color %lu: 0x%08x\n", i,
         icotools_read_u32( ico_bytes,
            ICO_HEADER_SZ + BMP_HEADER_SZ + (i * 4) ) );
   }

   /* Print bitmap data. */
   ico_px = &(ico_bytes[ICO_HEADER_SZ + BMP_HEADER_SZ + COLOR_TBL_SZ]);
   for( i = 0 ; ICO_W_PX * ICO_H_PX > i ; i++ ) {
      if( 0 != i % 2 ) {
         px_byte = ico_px[i / 2] & 0x0f;
      } else {
         px_byte = (ico_px[i / 2] >> 4) & 0x0f;
      }

      printf( "0x%01x ", px_byte );

      if( 0 == (i + 1) % ICO_W_PX ) {
         printf( "\n%02ld: ", (i / 8) + 1 );
      }
   }

   /* Print AND mask. */
   ico_px = &(ico_bytes[ICO_HEADER_SZ + BMP_HEADER_SZ + COLOR_TBL_SZ + 128]);
   for( i = 0 ; 64 > i ; i++ ) {
      if( 0 == i % 4 ) {
         printf( "\n" );
      }

      icotools_bprintf( ico_px[i] );
      printf( " " );

   }

   printf( "\n" );

cleanup:

   return retval;
}

