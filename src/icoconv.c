
#include "icotools.h"

/* Write an ICO TOC entry.
 */
size_t write_ico_entry( FILE* ico_file, struct ICOTOOLS_BMP_INFO* bmp ) {
   size_t ico_pos_out = 0;
   uint8_t byte_buf = 0;
   size_t ico_xor_mask_sz = 0;
   size_t ico_and_mask_sz = 0;

   ico_xor_mask_sz = icotools_xor_mask_sz( bmp->w_px,  bmp->h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp->h_px );

   icotools_write_u8( ico_file, bmp->w_px, byte_buf );
   icotools_write_u8( ico_file, bmp->h_px, byte_buf );
   icotools_write_u8( ico_file, bmp->pal_sz, byte_buf );
   icotools_write_u8( ico_file, 0, byte_buf );
   icotools_write_u16( ico_file, 1 /* color planes */, byte_buf );
   icotools_write_u16( ico_file, 4 /* bpp */, byte_buf );
   icotools_write_u32(
      ico_file, BMP_HEADER_SZ + (bmp->pal_sz * 4) + 
      ico_and_mask_sz + ico_xor_mask_sz, byte_buf );
   ico_pos_out = ftell( ico_file );
   icotools_write_u32( ico_file, 0 /* offset placeholder */, byte_buf );

   return ico_pos_out;
}

void write_bmp_header(
   FILE* ico_file, struct ICOTOOLS_BMP_INFO* bmp, uint8_t* color_tbl
) {
   uint8_t byte_buf = 0;
   size_t ico_and_mask_sz = 0;
   size_t ico_xor_mask_sz = 0;

   /* Write bitmap header. */
   icotools_write_u32( ico_file, 40, byte_buf );
   icotools_write_u32( ico_file, bmp->w_px, byte_buf );
   icotools_write_u32( ico_file, bmp->h_px * 2, byte_buf );
   icotools_write_u16( ico_file, 1 /* color planes */, byte_buf );
   icotools_write_u16( ico_file, 4 /* bpp */, byte_buf );
   icotools_write_u32( ico_file, 0 /* compression */, byte_buf );
   ico_xor_mask_sz = icotools_xor_mask_sz( bmp->w_px,  bmp->h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp->h_px );
   icotools_write_u32(
      ico_file, ico_xor_mask_sz + ico_and_mask_sz, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );
   icotools_write_u32( ico_file, bmp->pal_sz, byte_buf );
   icotools_write_u32( ico_file, 0, byte_buf );

   /* Copy color table. */
   fwrite( color_tbl, 1, bmp->pal_sz * 4, ico_file );
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

int convert_bmp_ico_data(
   FILE* ico_file, uint8_t* bmp_px_bytes, struct ICOTOOLS_BMP_INFO* bmp,
   uint8_t trans_pal_idx
) {
   size_t and_mask_byte_iter = 0;
   size_t i = 0;
   uint8_t color_byte = 0;
   uint8_t* ico_and_mask = NULL;
   uint8_t* ico_xor_mask = NULL;
   size_t ico_xor_mask_sz = 0;
   size_t ico_and_mask_sz = 0;
   int retval = 0;

   ico_xor_mask_sz = icotools_xor_mask_sz( bmp->w_px,  bmp->h_px );
   ico_and_mask_sz = icotools_and_mask_sz( bmp->h_px );

   ico_and_mask = calloc( 4, bmp->h_px ); /* 4-byte rows always! */
   if( NULL == ico_and_mask ) {
      retval = ICOTOOLS_ERR_ALLOC;
      goto cleanup;
   }

   ico_xor_mask = calloc( bmp->w_px / 2 /* 4bpp rows */, bmp->h_px );
   if( NULL == ico_xor_mask ) {
      retval = ICOTOOLS_ERR_ALLOC;
      goto cleanup;
   }

   for( i = 0 ; bmp->w_px * bmp->h_px > i ; i++ ) {
      /* Fill out the XOR mask (bitmap data). */

      if( trans_pal_idx == bmp_px_bytes[i] ) {
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
      if( trans_pal_idx == bmp_px_bytes[i] ) {
         /* 1 = transparent. */
         ico_and_mask[and_mask_byte_iter] |= 0x01;
      }
      if( 0 == (i + 1) % 8 ) {
         /* Move to a new byte every 8 pixels (since 1bpp). */
         and_mask_byte_iter++;
      }
      if( ((16 == bmp->w_px) && (0 == (i + 1) % 16)) ) {
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
   size_t ico_offsets[128];
   size_t prev_pos = 0;
   struct ICOTOOLS_BMP_INFO bmp_info[128];
   uint8_t trans_pal_idx = 0;
   uint8_t bmp_count = 0;
 
   if( 3 > argc ) {
      fprintf( stderr, "usage: %s [-t index] <bmp_file_in...> <ico_file_out>\n", argv[0] );
      fprintf( stderr, "\n" );
      fprintf( stderr, " -t index : use the color at [index] on the palette as transparent\n" );
      fprintf( stderr, "\n" );
      fprintf( stderr, "multiple input bitmaps will be muxed into a single ico\n" );
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
      if( 0 == strncmp( argv[i], "-t", 3 ) ) {
         /* Grab the new transparent palette color. */
         assert( argc - 1 > i + 1 );
         i++;
         trans_pal_idx = atoi( argv[i] );
         printf( "transparent color: %d\n", trans_pal_idx );
         continue;
      }

      /* Read bitmap file. */
      retval = icotools_read_file(
         argv[i], &(bmp_bytes[bmp_count]), &bmp_sz, 14 );
      if( 0 != retval ) {
         goto cleanup;
      }

      retval = icotools_verify_bmp(
         bmp_bytes[bmp_count], &(bmp_info[bmp_count]),
         ICOTOOLS_FLAG_VERIFY_BPP | ICOTOOLS_FLAG_VERIFY_DIMX );
      if( 0 != retval ) {
         goto cleanup;
      }

      printf( "reading bitmap %lu: %s\n", i, argv[i] );

      ico_offsets[bmp_count] = write_ico_entry(
         ico_file, &(bmp_info[bmp_count]) );

      bmp_count++;
   }

   /* Jump back up to the ICO header and write the image count. */
   prev_pos = ftell( ico_file );
   fseek( ico_file, 4, SEEK_SET );
   icotools_write_u16( ico_file, i - 1, byte_buf );
   fseek( ico_file, prev_pos, SEEK_SET );

   /* Begin writing the icon bitmaps. */
   for( i = 0 ; bmp_count > i ; i++ ) {

      printf( "writing bitmap: %lu\n", i );

      update_ico_entry_offset( ico_file, ico_offsets[i] );
      write_bmp_header( ico_file, &(bmp_info[i]),
         &(bmp_bytes[i][BMP_HEADER_SZ]) );

      retval = convert_bmp_ico_data( ico_file, 
         &(bmp_bytes[i][BMP_HEADER_SZ + (bmp_info[i].pal_sz * 4)]),
         &(bmp_info[i]), trans_pal_idx );
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

