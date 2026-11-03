/*
** Copyright (c) 1996, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
** $Revision: 3 $
** $Date: 3/17/97 9:33p $
*/
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <glide.h>

int main( int argc, char **argv )
{
  GrMipMapId_t mmid1, mmid2;
  Gu3dfInfo info1, info2;
  static GrHwConfiguration hwconfig;
  GrVertex vtx[4];
  int width, height;
  float smult, tmult;

  if((argc != 2) && (argc != 3) )
    {
      fprintf( stderr, "Usage: view3df file1.3df [file2.3df]\n" );
      exit( -1 );
    }

  grGlideInit();
  
  if ( !grSstQueryHardware( &hwconfig ) )
    {
      fprintf( stderr, "main: grSstQueryHardware failed!" );
      exit( -1 );
    }

  grSstSelect( 0 );
  if ( !grSstWinOpen( 0,
                      GR_RESOLUTION_640x480,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 0 ) )
  {
    fprintf( stderr, "main: grSstOpen failed!" );
    exit( -1 );
  }

  grRenderBuffer( GR_BUFFER_FRONTBUFFER );

  /*********/
  if( !gu3dfGetInfo( argv[1], &info1 ) )
    {
      fprintf( stderr, "Not able to get info on %s\n", argv[1] );
      exit( -1 );
    }
  info1.data = malloc( info1.mem_required );
  gu3dfLoad( argv[1], &info1 );

  mmid1 = guTexAllocateMemory( 0, 0x3, 
                              info1.header.width, info1.header.height,
                              info1.header.format,
                              GR_MIPMAP_NEAREST_DITHER,
                              info1.header.small_lod, info1.header.large_lod,
                              info1.header.aspect_ratio,
                              GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                              GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR,
                              0.0F,
                              FXFALSE );

  guTexDownloadMipMap( mmid1, info1.data, &info1.table.nccTable );

  /*********/
  if (argc == 3) {
  if( !gu3dfGetInfo( argv[2], &info2 ) )
    {
      fprintf( stderr, "Not able to get info on %s\n", argv[2] );
      exit( -1 );
    }
  info2.data = malloc( info2.mem_required );
  gu3dfLoad( argv[2], &info2 );

  mmid2 = guTexAllocateMemory( 0, 0x3, 
                              info2.header.width, info2.header.height,
                              info2.header.format,
                              GR_MIPMAP_NEAREST_DITHER,
                              info2.header.small_lod, info2.header.large_lod,
                              info2.header.aspect_ratio,
                              GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                              GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR,
                              0.0F,
                              FXFALSE );

  guTexDownloadMipMap( mmid2, info2.data, &info2.table.nccTable );
  }

  /*********/
  grTexCombineFunction( GR_TMU0, GR_TEXTURECOMBINE_DECAL );
  guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );

  /*********/
  guTexSource( mmid1 );

  width  = info1.header.width;
  height = info1.header.height;

  switch( info1.header.aspect_ratio )
    {
    case GR_ASPECT_8x1:
      smult = 1.0f;
      tmult = 0.125f;
      break;
    case GR_ASPECT_4x1:
      smult = 1.0f;
      tmult = 0.25f;
      break;
    case GR_ASPECT_2x1:
      smult = 1.0f;
      tmult = 0.5f;
      break;
    case GR_ASPECT_1x1:
      smult = 1.0f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x2:
      smult = 0.5f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x4:
      smult = 0.25f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x8:
      smult = 0.125f;
      tmult = 1.0f;
      break;
    }

  vtx[0].x = 0.0f;
  vtx[0].y = 0.0f + 100.0f;
  vtx[0].oow = 1.0f;
  vtx[0].tmuvtx[0].sow = 0.0f;
  vtx[0].tmuvtx[0].tow = 0.0f;

  vtx[1].x = width - 1.0f;
  vtx[1].y = 0.0f + 100.0f;
  vtx[1].oow = 1.0f;
  vtx[1].tmuvtx[0].sow = 255.0f * smult;
  vtx[1].tmuvtx[0].tow = 0.0f;

  vtx[2].x = width - 1.0f;
  vtx[2].y = height - 1.0f + 100.0f;
  vtx[2].oow = 1.0f;
  vtx[2].tmuvtx[0].sow = 255.0f * smult;
  vtx[2].tmuvtx[0].tow = 255.0f * tmult;

  vtx[3].x = 0.0f;
  vtx[3].y = height - 1.0f + 100.0f;
  vtx[3].oow = 1.0f;
  vtx[3].tmuvtx[0].sow = 0.0f;
  vtx[3].tmuvtx[0].tow = 255.0f * tmult;

  grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
  grDrawTriangle( &vtx[0], &vtx[2], &vtx[3] );

        /****************/
  if (argc == 3) {
  guTexSource( mmid2 );
  width  = info2.header.width;
  height = info2.header.height;

  switch( info2.header.aspect_ratio )
    {
    case GR_ASPECT_8x1:
      smult = 1.0f;
      tmult = 0.125f;
      break;
    case GR_ASPECT_4x1:
      smult = 1.0f;
      tmult = 0.25f;
      break;
    case GR_ASPECT_2x1:
      smult = 1.0f;
      tmult = 0.5f;
      break;
    case GR_ASPECT_1x1:
      smult = 1.0f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x2:
      smult = 0.5f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x4:
      smult = 0.25f;
      tmult = 1.0f;
      break;
    case GR_ASPECT_1x8:
      smult = 0.125f;
      tmult = 1.0f;
      break;
    }

  vtx[0].x = 0.0f + 258.0f;
  vtx[0].y = 0.0f + 100.0f;
  vtx[0].oow = 1.0f;
  vtx[0].tmuvtx[0].sow = 0.0f;
  vtx[0].tmuvtx[0].tow = 0.0f;

  vtx[1].x = width - 1 + 258.0f;
  vtx[1].y = 0.0f + 100.0f;
  vtx[1].oow = 1.0f;
  vtx[1].tmuvtx[0].sow = 255.0f * smult;
  vtx[1].tmuvtx[0].tow = 0.0f;

  vtx[2].x = width - 1 + 258.0f;
  vtx[2].y = height - 1 + 100.0f;
  vtx[2].oow = 1.0f;
  vtx[2].tmuvtx[0].sow = 255.0f * smult;
  vtx[2].tmuvtx[0].tow = 255.0f * tmult;

  vtx[3].x = 0.0f + 258.0f;
  vtx[3].y = height - 1 + 100.0f;
  vtx[3].oow = 1.0f;
  vtx[3].tmuvtx[0].sow = 0.0f;
  vtx[3].tmuvtx[0].tow = 255.0f * tmult;

  grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
  grDrawTriangle( &vtx[0], &vtx[2], &vtx[3] );
  }

  printf( "Hit a key to exit\n" );
  fflush( stdout );
  getch();

  grGlideShutdown();

  return 0;
}
