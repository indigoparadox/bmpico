
#include "icotools.h"

void print_bmp_palette(
   const struct ICOTOOLS_BMP_INFO* bmp_info,
   const struct ICOTOOLS_ICO_ENTRY* ico_info,
   const uint8_t* ico_bytes
) {
   size_t i = 0;

   /* Print bitmap header info. */
   printf( "bitmap width: %u\n", bmp_info->w_px );
   printf( "bitmap height: %u\n", bmp_info->h_px );
   printf( "bitmap color planes: %u\n", bmp_info->color_planes );
   printf( "bitmap bpp: %u\n", bmp_info->bpp );
   printf( "bitmap compression: %u\n", bmp_info->compression );
   printf( "bitmap size: %u\n", bmp_info->image_sz );
   printf( "bitmap ppm_h: %u\n", bmp_info->ppm_h );
   printf( "bitmap ppm_v: %u\n", bmp_info->ppm_v );
   printf( "bitmap colors: %u\n", bmp_info->pal_sz );
   printf( "bitmap imp colors: %u\n", bmp_info->important_c );

   /* Print color palette. */
   for( i = 0 ; bmp_info->pal_sz > i ; i++ ) {
      printf( "color %lu of %u: 0x%08x\n", i, bmp_info->pal_sz,
         icotools_read_u32( ico_bytes,
            ico_info->bmp_offset + BMP_HEADER_SZ + (i * 4) ) );
   }
}

int main( int argc, char* argv[] ) {
   int retval = 0;
   uint8_t* ico_bytes = NULL;
   uint8_t* ico_px = NULL;
   uint16_t ico_field_res = 0;
   uint16_t ico_field_type = 0;
   uint16_t ico_field_num_imgs = 0;
   size_t ico_sz = 0;
   size_t i = 0;
   size_t j = 0;
   uint8_t px_byte = 0;
   size_t ico_xor_mask_sz = 0;
   size_t ico_and_mask_sz = 0;
   struct ICOTOOLS_BMP_INFO bmp_info[128];
   struct ICOTOOLS_ICO_ENTRY ico_info[128];
   
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
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   ico_field_type = icotools_read_u16( ico_bytes, 2 );
   if( 1 != ico_field_type ) {
      fprintf( stderr, "invalid: field type: %u\n", ico_field_type );
      retval = ICOTOOLS_ERR_VERIFY;
      goto cleanup;
   }

   ico_field_num_imgs = icotools_read_u16( ico_bytes, 4 );
   printf( "found %u icons in file...\n", ico_field_num_imgs );

   /* Begin reading ico entries. */
   for( i = 0 ; ico_field_num_imgs > i ; i++ ) {

      icotools_verify_ico_entry(
         &(ico_bytes[ICO_FILE_HEADER_SZ + (i * ICO_ENTRY_HEADER_SZ)]),
         &(ico_info[i]) );

      /* TODO: Sanity check on bitmap offset/size. */

      printf( "ico %lu data at %u bytes, %u bytes long\n",
         i, ico_info[i].bmp_offset, ico_info[i].bmp_sz );

      retval = icotools_verify_bmp(
         &(ico_bytes[ico_info[i].bmp_offset]), &(bmp_info[i]), 0 );
      if( 0 != retval ) {
         goto cleanup;
      }

      if( bmp_info[i].bpp !=  ico_info[i].bpp ) { 
         fprintf( stderr, "invalid: color depth mismatch: %u vs %u\n",
            bmp_info[i].bpp, ico_info[i].bpp );
         retval = ICOTOOLS_ERR_VERIFY;
         goto cleanup;
      }

      if( bmp_info[i].pal_sz !=  ico_info[i].pal_sz ) { 
         fprintf( stderr, "invalid: palette size mismatch: %u vs %u\n",
            bmp_info[i].pal_sz, ico_info[i].pal_sz );
         retval = ICOTOOLS_ERR_VERIFY;
         goto cleanup;
      }

      print_bmp_palette( &(bmp_info[i]), &(ico_info[i]), ico_bytes );

      printf( "\nXOR mask:\n" );

      /* Print bitmap data. */
      ico_xor_mask_sz = icotools_xor_mask_sz(
         bmp_info[i].w_px, bmp_info[i].h_px );
      ico_px = &(ico_bytes[
         ico_info[i].bmp_offset + BMP_HEADER_SZ + (bmp_info[i].pal_sz * 4)]);
      printf( "\n00: " );
      for( j = 0 ; ico_xor_mask_sz > j ; j++ ) {
         if( 0 != j % 2 ) {
            px_byte = ico_px[j / 2] & 0x0f;
         } else {
            px_byte = (ico_px[j / 2] >> 4) & 0x0f;
         }

         printf( "0x%01x ", px_byte );

         if( 0 == (j + 1) % bmp_info[i].w_px ) {
            printf( "\n%02ld: ", (i / bmp_info[i].w_px) + 1 );
         }
      }

      printf( "\nAND mask:\n" );

      /* Print AND mask. */
      ico_px = &(ico_bytes[
         ico_info[i].bmp_offset + BMP_HEADER_SZ +
         (bmp_info[i].pal_sz * 4) + ico_xor_mask_sz]);
      ico_and_mask_sz = 4 * bmp_info[i].h_px;
      for( j = 0 ; ico_and_mask_sz > j ; j++ ) {
         if( 0 == j % 4 ) {
            printf( "\n%02ld: ", j / 4 );
         }

         icotools_bprintf( ico_px[j] );
         printf( " " );
      }

      printf( "\n\n" );
   }

cleanup:

   return retval;
}

