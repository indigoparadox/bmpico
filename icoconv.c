
#include "icotools.h"

/* Write an ICO TOC entry.
 */
size_t write_ico_entry(
   FILE* ico_file, uint8_t bmp_w_px, uint8_t bmp_h_px, uint8_t bmp_bpp
) {
   size_t ico_pos_out = 0;
   uint8_t byte_buf = 0;
   size_t ico_xor_mask_sz = 0;
   size_t ico_and_mask_sz = 0;

   ico_xor_mask_sz = icotools_xor_mask_sz( bmp_w_px,  bmp_h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp_h_px );

   icotools_write_u8( ico_file, bmp_w_px, byte_buf );
   icotools_write_u8( ico_file, bmp_h_px, byte_buf );
   icotools_write_u8( ico_file, bmp_bpp, byte_buf );
   icotools_write_u8( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u32(
      ico_file, BMP_HEADER_SZ + COLOR_TBL_SZ + 
      ico_and_mask_sz + ico_xor_mask_sz, byte_buf );
   ico_pos_out = ftell( ico_file );
   icotools_write_u32( ico_file, 0 /* offset placeholder */, byte_buf );

   return ico_pos_out;
}

void write_bmp_header(
   FILE* ico_file, uint8_t bmp_w_px, uint8_t bmp_h_px, uint8_t* color_tbl
) {
   uint8_t byte_buf = 0;
   size_t ico_and_mask_sz = 0;
   size_t ico_xor_mask_sz = 0;

   /* Write bitmap header. */
   icotools_write_u32( ico_file, 40, byte_buf );
   icotools_write_u32( ico_file, bmp_w_px, byte_buf );
   icotools_write_u32( ico_file, bmp_h_px * 2, byte_buf );
   icotools_write_u16( ico_file, 1 /* color planes */, byte_buf );
   icotools_write_u16( ico_file, 4 /* bpp */, byte_buf );
   icotools_write_u32( ico_file, 0 /* compression */, byte_buf );
   ico_xor_mask_sz = icotools_xor_mask_sz( bmp_w_px,  bmp_h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp_h_px );
   icotools_write_u32(
      ico_file, ico_xor_mask_sz + ico_and_mask_sz, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, 16 /* palette sz */, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );

   /* Copy color table. */
   fwrite( color_tbl, 1, COLOR_TBL_SZ, ico_file );
}

/* Go back to the given offset and write the current file offset as a 32-bit
 * integer there, then return to the offset when called.
 */
void update_ico_entry_offset( FILE* ico_file, size_t offset_pos ) {
   uint32_t prev_pos = 0;
   uint8_t byte_buf = 0;

   prev_pos = ftell( ico_file );

   fseek( ico_file, offset_pos, SEEK_SET );
   icotools_write_u32( ico_file, prev_pos, byte_buf );
   fseek( ico_file, prev_pos, SEEK_SET );
}

int verify_bmp(
   uint8_t* bmp_bytes, uint8_t* bmp_w_px, uint8_t* bmp_h_px, uint8_t* bmp_bpp
) {
   int retval = 0;

   if( 40 != icotools_read_u16( bmp_bytes, 0 ) ) {
      fprintf( stderr, "invalid bitmap header!\n" );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   *bmp_bpp = icotools_read_u16( bmp_bytes, BMPINFO_OFFSET_BPP );
   if( 8 != *bmp_bpp ) {
      fprintf( stderr, "invalid bitmap bpp: %u\n", *bmp_bpp );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   *bmp_w_px = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_WIDTH );
   *bmp_h_px = icotools_read_u32( bmp_bytes, BMPINFO_OFFSET_HEIGHT );

   if( 16 != *bmp_w_px && 32 != *bmp_w_px ) {
      fprintf( stderr, "invalid bitmap width: %u\n", *bmp_w_px );
      retval = ICOTOOLS_ERR_READ;
      goto cleanup;
   }

   if( 16 != *bmp_h_px && 32 != *bmp_h_px ) {
      fprintf( stderr, "invalid bitmap height: %u\n", *bmp_h_px );
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

cleanup:
   return retval;
}

int convert_bmp_ico_data(
   FILE* ico_file, uint8_t* bmp_px_bytes, uint8_t bmp_w_px, uint8_t bmp_h_px
) {
   size_t and_mask_byte_iter = 0;
   size_t i = 0;
   uint8_t color_byte = 0;
   uint8_t* ico_and_mask = NULL;
   uint8_t* ico_xor_mask = NULL;
   size_t ico_xor_mask_sz = 0;
   size_t ico_and_mask_sz = 0;
   int retval = 0;

   ico_xor_mask_sz = icotools_xor_mask_sz( bmp_w_px,  bmp_h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp_h_px );

   ico_and_mask = calloc( 4, bmp_h_px ); /* 4-byte rows always! */
   if( NULL == ico_and_mask ) {
      retval = ICOTOOLS_ERR_ALLOC;
      goto cleanup;
   }

   ico_xor_mask = calloc( bmp_w_px / 2 /* 4bpp rows */, bmp_h_px );
   if( NULL == ico_xor_mask ) {
      retval = ICOTOOLS_ERR_ALLOC;
      goto cleanup;
   }

   for( i = 0 ; bmp_w_px * bmp_h_px > i ; i++ ) {
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
      if( ((16 == bmp_w_px) && (0 == (i + 1) % 16)) ) {
         /* This is still technically bitmap data, so fill out rows to be
          * multiples of 4!
          */
         and_mask_byte_iter++;
         and_mask_byte_iter++;
      }
   }

   /* Write bitmap data to icon file. */
   fwrite( ico_xor_mask, 1, ico_xor_mask_sz, ico_file );
   fwrite( ico_and_mask, 1, ico_and_mask_sz, ico_file );

cleanup:

   if( NULL != ico_and_mask ) {
      free( ico_and_mask );
   }

   if( NULL != ico_xor_mask ) {
      free( ico_xor_mask );
   }

   return retval;
}

int main( int argc, char* argv[] ) {
   int retval = 0;
   FILE* ico_file = NULL;
   uint8_t* bmp_bytes[128];
   size_t bmp_sz = 0;
   uint8_t byte_buf = 0;
   size_t i = 0;
   uint8_t* bmp_px_bytes = NULL;
   uint8_t bmp_w_px[128];
   uint8_t bmp_h_px[128];
   size_t ico_offsets[128];
   uint8_t bmp_bpp[128];
   size_t prev_pos = 0;
 
   if( 3 > argc ) {
      fprintf( stderr, "usage: %s <bmp_file_in> <ico_file_out>\n", argv[0] );
      retval = 1;
      goto cleanup;
   }

   /* Open ico file for writing. */
   /* TODO: Use a temporary file that can be deleted if bitmap verify fails.
    */
   ico_file = fopen( argv[argc - 1], "wb" );
   if( NULL == ico_file ) {
      retval = ICOTOOLS_ERR_FILE;
      goto cleanup;
   }

   /* Write icon header. */
   icotools_write_u16( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 1, byte_buf );
   icotools_write_u16( ico_file, 0, byte_buf );

   /* Skip program name and output file. */
   for( i = 1 ; argc - 1 > i ; i++ ) {

      /* Read bitmap file. */
      retval = icotools_read_file( argv[i], &(bmp_bytes[i]), &bmp_sz, 14 );
      if( 0 != retval ) {
         goto cleanup;
      }

      retval = verify_bmp(
         bmp_bytes[i], &(bmp_w_px[i]), &(bmp_h_px[i]), &(bmp_bpp[i]) );
      if( 0 != retval ) {
         goto cleanup;
      }

      ico_offsets[i] =
         write_ico_entry( ico_file, bmp_w_px[i], bmp_h_px[i], bmp_bpp[i] );
   }

   /* Jump back up to the ICO header and write the image count. */
   prev_pos = ftell( ico_file );
   fseek( ico_file, 4, SEEK_SET );
   icotools_write_u16( ico_file, i - 1, byte_buf );
   fseek( ico_file, prev_pos, SEEK_SET );

   /* Begin writing the icon bitmaps. */
   for( i = 1 ; argc - 1 > i ; i++ ) {

      update_ico_entry_offset( ico_file, ico_offsets[i] );
      write_bmp_header( ico_file, bmp_w_px[i], bmp_h_px[i],
         &(bmp_bytes[i][BMP_HEADER_SZ]) );

      retval = convert_bmp_ico_data( ico_file, 
         &(bmp_bytes[i][BMP_HEADER_SZ + COLOR_TBL_SZ]),
         bmp_w_px[i], bmp_h_px[i] );
      if( 0 != retval ) {
         goto cleanup;
      }
   }
      
cleanup:

   if( NULL != ico_file ) {
      fclose( ico_file );
   }

   return retval;
}

