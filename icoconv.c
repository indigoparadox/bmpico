
#include "icotools.h"

int main( int argc, char* argv[] ) {
   int retval = 0;
   FILE* ico_file = NULL;
   uint8_t* bmp_bytes = 0;
   size_t bmp_sz = 0;
   uint8_t byte_buf = 0;
   size_t i = 0;
   uint8_t ico_bmp_bytes[(ICO_W_PX / 2) * ICO_H_PX];
   uint8_t ico_and_mask[64];
   uint8_t ico_xor_mask[128];
   uint8_t* bmp_px_bytes = NULL;
   uint8_t color_byte = 0;
   size_t and_mask_byte_iter = 63;
   uint8_t bmp_w = 16;
   uint8_t bmp_h = 16;
   uint8_t bmp_bpp = 16;
 
   if( 3 > argc ) {
      fprintf( stderr, "usage: %s <bmp_file_in> <ico_file_out>\n", argv[0] );
      retval = 1;
      goto cleanup;
   }

   memset( ico_and_mask, '\0', 64 );
   memset( ico_xor_mask, '\0', 128 );

   /* Read bitmap file. */
   retval = icotools_read_file( argv[1], &bmp_bytes, &bmp_sz, 14 );
   if( 0 != retval ) {
      goto cleanup;
   }

   if( 40 != icotools_read_u16( bmp_bytes, 0 ) ) {
      fprintf( stderr, "invalid bitmap header!\n" );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( 8 != icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_BPP ) ) {
      fprintf( stderr,
         "invalid bitmap bpp: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_BPP ) );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( ICO_W_PX != icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_WIDTH ) ) {
      fprintf( stderr,
         "invalid bitmap width: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_WIDTH ) );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( ICO_H_PX != icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_HEIGHT ) ) {
      fprintf( stderr,
         "invalid bitmap height: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_HEIGHT ) );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( 0 != icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_CMP ) ) {
      fprintf( stderr,
         "invalid bitmap compression: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_CMP ) );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( 16 != icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_PAL_SZ ) ) {
      fprintf( stderr,
         "invalid bitmap palette size: %u\n",
         icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_PAL_SZ ) );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   /* Open ico file for writing. */
   ico_file = fopen( argv[2], "wb" );
   if( NULL == ico_file ) {
      retval = ICOTOOLS_ERR_FILE;
      goto cleanup;
   }

   /* Write icon header. */
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 1, byte_buf );
   icotools_write_u16( ico_file, 1, byte_buf );

   /* Write ico image entry. */
   icotools_write_u8( ico_file, bmp_w, byte_buf );
   icotools_write_u8( ico_file, bmp_h, byte_buf );
   icotools_write_u8( ico_file, bmp_bpp, byte_buf );
   icotools_write_u8( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u32(
      ico_file, BMP_HEADER_SZ + COLOR_TBL_SZ + ICO_DATA_SZ, byte_buf );
   icotools_write_u32( ico_file, ftell( ico_file ) + 4, byte_buf );

   /* Write bitmap header. */
   icotools_write_u32( ico_file, 40, byte_buf );
   icotools_write_u32( ico_file, ICO_W_PX, byte_buf );
   icotools_write_u32( ico_file, ICO_H_PX * 2, byte_buf );
   icotools_write_u16( ico_file, 1 /* color planes */, byte_buf );
   icotools_write_u16( ico_file, 4 /* bpp */, byte_buf );
   icotools_write_u32( ico_file, 0 /* compression */, byte_buf );
   icotools_write_u32( ico_file, ICO_DATA_SZ, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, 16 /* palette sz */, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );

   /* Copy color table. */
   fwrite( &(bmp_bytes[BMP_HEADER_SZ]), 1, COLOR_TBL_SZ, ico_file );

   bmp_px_bytes = &(bmp_bytes[BMP_HEADER_SZ + COLOR_TBL_SZ]);
   and_mask_byte_iter = 0;
   for( i = 0 ; ICO_W_PX * ICO_H_PX > i ; i++ ) {
      /* Fill out the XOR mask (bitmap data). */

      if( ICO_TRANSPARENT_COLOR == bmp_px_bytes[i] ) {
         /* In addition to having a 0 in the AND mask below, the transparent
          * color must also be set to 0!
          */
         color_byte = 0;
      } else {
         /* Grab the byte for this bitmap pixel. */
         color_byte = bmp_px_bytes[i];
      }

      if( 0 == i % 2 ) {
         /* Even pixels. */
         ico_xor_mask[i / 2 /* 4bpp */] |= ((color_byte & 0x0f) << 4);
      } else {
         /* Odd pixels. */
         ico_xor_mask[i / 2 /* 4bpp */] |= (color_byte & 0x0f);
      }

      /* Fill out the AND mask (transparency data). */
      if( 0 != i % 8 ) {
         ico_and_mask[and_mask_byte_iter] <<= 1;
      }
      if( ICO_TRANSPARENT_COLOR == bmp_px_bytes[i] ) {
         /* 1 = transparent. */
         ico_and_mask[and_mask_byte_iter] |= 0x01;
      }
      if( 0 == (i + 1) % 8 ) {
         /* Move to a new byte every 8 pixels (since 1bpp). */
         and_mask_byte_iter++;
      }
      if( 0 == (i + 1) % 16 ) {
         /* This is still technically bitmap data, so fill out rows to be
          * multiples of 4!
          */
         and_mask_byte_iter++;
         and_mask_byte_iter++;
      }
   }
   fwrite( ico_xor_mask, 1, 128, ico_file );
 
   fwrite( ico_and_mask, 1, 64, ico_file );

cleanup:

   if( NULL != ico_file ) {
      fclose( ico_file );
   }

   return retval;
}

