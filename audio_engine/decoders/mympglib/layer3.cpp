
/*
 * Mpeg Layer-3 audio decoder
 * --------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp.
 * All rights reserved. See also 'README'
 */

#include "mympg123.h"
#include "mympglib.h"
#include "MinMax.h"
#include "huffman.h"

#define MPEG1

static real ispow[8207];
static real aa_ca[8],aa_cs[8];
static real COS1[12][6];
static real win[4][36];
static real win1[4][36];
static real gainpow2[256+118+4];

static real COS9[9];
static real COS6_1,COS6_2;
static real tfcos36[9];

static real tfcos12[3];

#ifdef NEW_DCT9
static real cos9[3],cos18[3];
#endif

struct bandInfoStruct
{
    int longIdx[23];
    int longDiff[22];
    int shortIdx[14];
    int shortDiff[13];
};

static unsigned int longLimit[9][23];
static unsigned int shortLimit[9][14];

static const struct bandInfoStruct bandInfo[9] =
{

    /* MPEG 1.0 */
    {   {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
        {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
        {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
        {4,4,4,4,6,8,10,12,14,18,22,30,56}
    },

    {   {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
        {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
        {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
        {4,4,4,4,6,6,10,12,14,16,20,26,66}
    },

    {   {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
        {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26},
        {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3},
        {4,4,4,4,6,8,12,16,20,26,34,42,12}
    },

    /* MPEG 2.0 */
    {   {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
        {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
        {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3},
        {4,4,4,6,6,8,10,14,18,26,32,42,18 }
    },

    /* changed 19th value fropm 330 to 332 */
    {   {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,332,394,464,540,576},
        {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,52,64,70,76,36 },
        {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3},
        {4,4,4,6,8,10,12,14,18,24,32,44,12 }
    },

    {   {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
        {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
        {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
        {4,4,4,6,8,10,12,14,18,24,30,40,18 }
    },
    /* MPEG 2.5 */
    {   {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
        {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
        {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
        {4,4,4,6,8,10,12,14,18,24,30,40,18}
    },
    {   {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
        {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
        {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
        {4,4,4,6,8,10,12,14,18,24,30,40,18}
    },
    {   {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576},
        {12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2},
        {0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576},
        {8,8,8,12,16,20,24,28,36,2,2,2,26}
    },
};

static int mapbuf0[9][152];
static int mapbuf1[9][156];
static int mapbuf2[9][44];
static int *map[9][3];
static int *mapend[9][3];

static unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
static unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

static real tan1_1[32],tan2_1[32],tan1_2[32],tan2_2[32];
static real pow1_1[2][32],pow2_1[2][32],pow1_2[2][32],pow2_2[2][32];

/*
 * init tables for layer-3
 */
void __fastcall
init_layer3(const unsigned int down_sample_sblimit)
{
    int i,j,k,l;

    for(i=-256; i<118+4; i++)
    {
        gainpow2[i+256] = pow((double)2.0,-0.25 * (double) (i+210) );
    }

    for(i=0; i<8207; i++)
        ispow[i] = pow((double)i,(double)4.0/3.0);

    for (i=0; i<8; i++)
    {
        static const double Ci[8]= {-0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142,-0.0037};
//    double sq=sqrt(1.0+Ci[i]*Ci[i]);  // !
        /*register*/ double sq = sqrt(1.0 + pow(Ci[i], 2.0));
        aa_cs[i] = 1.0/sq;
        aa_ca[i] = Ci[i]/sq;
    }

    for(i=0; i<18; i++)
    {
//    win[0][i]    = win[1][i]    = 0.5 * sin( M_PI / 72.0 * (double) (2*(i+0) +1) ) / cos ( M_PI * (double) (2*(i+0) +19) / 72.0 );
        win[0][i]    = win[1][i]    = 0.5 * sin(M_PI * (double)(2*(i+0) +1) / 72.0) / cos(M_PI * (double)(2*(i+0)+19) / 72.0);
//    win[0][i+18] = win[3][i+18] = 0.5 * sin( M_PI / 72.0 * (double) (2*(i+18)+1) ) / cos ( M_PI * (double) (2*(i+18)+19) / 72.0 );
        win[0][i+18] = win[3][i+18] = 0.5 * sin(M_PI * (double)(2*(i+18)+1) / 72.0) / cos(M_PI * (double)(2*(i+18)+19) / 72.0);
    }
    for(i=0; i<6; i++)
    {
        win[1][i+18] = 0.5 / cos ( M_PI * (double) (2*(i+18)+19) / 72.0 );
        win[3][i+12] = 0.5 / cos ( M_PI * (double) (2*(i+12)+19) / 72.0 );
//    win[1][i+24] = 0.5 * sin( M_PI / 24.0 * (double) (2*i+13) ) / cos ( M_PI * (double) (2*(i+24)+19) / 72.0 );
        win[1][i+24] = 0.5 * sin(M_PI * (double)(2*i+13) / 24.0) / cos(M_PI * (double)(2*(i+24)+19) / 72.0);
        win[1][i+30] = win[3][i] = 0.0;
//    win[3][i+6 ] = 0.5 * sin( M_PI / 24.0 * (double) (2*i+1) )  / cos ( M_PI * (double) (2*(i+6 )+19) / 72.0 );
        win[3][i+6 ] = 0.5 * sin(M_PI * (double)(2*i+1) / 24.0) / cos(M_PI * (double)(2*(i+6 )+19) / 72.0);
    }

    for(i=0; i<9; i++)
//    COS9[i] = cos( M_PI / 18.0 * (double) i);  // !
        COS9[i] = cos(M_PI * (double)i / 18.0);

    for(i=0; i<9; i++)
        tfcos36[i] = 0.5 / cos ( M_PI * (double) (i*2+1) / 36.0 );
    for(i=0; i<3; i++)
        tfcos12[i] = 0.5 / cos ( M_PI * (double) (i*2+1) / 12.0 );

//  COS6_1 = cos( M_PI / 6.0 * (double) 1);  // !
    COS6_1 = cos(M_PI /* * (double)1.0 */ / 6.0);
//  COS6_2 = cos( M_PI / 6.0 * (double) 2);  // !
    COS6_2 = cos(M_PI * (double)2.0 / 6.0);

#ifdef NEW_DCT9
    cos9[0]  = cos(/* 1.0* */ M_PI/9.0);
    cos9[1]  = cos(5.0*M_PI/9.0);
    cos9[2]  = cos(7.0*M_PI/9.0);
    cos18[0] = cos(/* 1.0* */ M_PI/18.0);
    cos18[1] = cos(11.0*M_PI/18.0);
    cos18[2] = cos(13.0*M_PI/18.0);
#endif

    for(i=0; i<12; i++)
    {
//    win[2][i]  = 0.5 * sin( M_PI / 24.0 * (double) (2*i+1) ) / cos ( M_PI * (double) (2*i+7) / 24.0 );  // !
        win[2][i] = 0.5 * sin(M_PI * (double)(2*i+1) / 24.0) / cos(M_PI * (double)(2*i+7) / 24.0);
        for(j=0; j<6; j++)
//      COS1[i][j] = cos( M_PI / 24.0 * (double) ((2*i+7)*(2*j+1)) );  // !
            COS1[i][j] = cos(M_PI * (double)((2*i+7)*(2*j+1)) / 24.0);
    }

    for(j=0; j<4; j++)
    {
        static const int len[4] = { 36,36,12,36 };
        for(i=0; i<len[j]; i+=2)
            win1[j][i] = + win[j][i];
        for(i=1; i<len[j]; i+=2)
            win1[j][i] = - win[j][i];
    }

    for(i=0; i<32; i++)
    {
        /*register*/ double t = tan( (double) i * M_PI / 12.0 );
        tan1_1[i] = t / (1.0+t);
        tan2_1[i] = 1.0 / (1.0 + t);
        tan1_2[i] = M_SQRT2 * t / (1.0+t);
        tan2_2[i] = M_SQRT2 / (1.0 + t);

        for(j=0; j<2; j++)
        {
            /*register*/ double base = pow(2.0,-0.25*(j+1.0));
            /*register*/ double p1=1.0,p2=1.0;
            if(i > 0)
            {
                if( i & 1 )
                    p1 = pow(base,(i+1.0)*0.5);
                else
                    p2 = pow(base,i*0.5);
            }
            pow1_1[j][i] = p1;
            pow2_1[j][i] = p2;
            pow1_2[j][i] = M_SQRT2 * p1;
            pow2_2[j][i] = M_SQRT2 * p2;
        }
    }

    for(j=0; j<9; j++)
    {
        const struct bandInfoStruct *bi = &bandInfo[j];
        int *mp;
        int cb,lwin;
        const int *bdf;

        mp = map[j][0] = mapbuf0[j];
        bdf = bi->longDiff;
        for(i=0,cb = 0; cb < 8 ; cb++,i+=*bdf++)
        {
            *mp++ = (*bdf) >> 1;
            *mp++ = i;
            *mp++ = 3;
            *mp++ = cb;
        }
        bdf = bi->shortDiff+3;
        for(cb=3; cb<13; cb++)
        {
            int l = (*bdf++) >> 1;
            for(lwin=0; lwin<3; lwin++)
            {
                *mp++ = l;
                *mp++ = i + lwin;
                *mp++ = lwin;
                *mp++ = cb;
            }
            i += 6*l;
        }
        mapend[j][0] = mp;

        mp = map[j][1] = mapbuf1[j];
        bdf = bi->shortDiff+0;
        for(i=0,cb=0; cb<13; cb++)
        {
            int l = (*bdf++) >> 1;
            for(lwin=0; lwin<3; lwin++)
            {
                *mp++ = l;
                *mp++ = i + lwin;
                *mp++ = lwin;
                *mp++ = cb;
            }
            i += 6*l;
        }
        mapend[j][1] = mp;

        mp = map[j][2] = mapbuf2[j];
        bdf = bi->longDiff;
        for(cb = 0; cb < 22 ; cb++)
        {
            *mp++ = (*bdf++) >> 1;
            *mp++ = cb;
        }
        mapend[j][2] = mp;

    }

    for(j=0; j<9; j++)
    {
        for(i=0; i<23; i++)
        {
            longLimit[j][i] =
                Min((unsigned int)((bandInfo[j].longIdx[i] - 1 + 8) / 18 + 1), down_sample_sblimit);
        }
        for(i=0; i<14; i++)
        {
            shortLimit[j][i] =
                Min((unsigned int)((bandInfo[j].shortIdx[i] - 1) / 18 + 1), down_sample_sblimit);
        }
    }

    for(i=0; i<5; i++)
    {
        for(j=0; j<6; j++)
        {
            for(k=0; k<6; k++)
            {
                int n = k + j * 6 + i * 36;
                i_slen2[n] = i|(j<<3)|(k<<6)|(3<<12);
            }
        }
    }
    for(i=0; i<4; i++)
    {
        for(j=0; j<4; j++)
        {
            for(k=0; k<4; k++)
            {
                int n = k + j * 4 + i * 16;
                i_slen2[n+180] = i|(j<<3)|(k<<6)|(4<<12);
            }
        }
    }
    for(i=0; i<4; i++)
    {
        for(j=0; j<3; j++)
        {
            int n = j + i * 3;
            i_slen2[n+244] = i|(j<<3) | (5<<12);
            n_slen2[n+500] = i|(j<<3) | (2<<12) | (1<<15);
        }
    }

    for(i=0; i<5; i++)
    {
        for(j=0; j<5; j++)
        {
            for(k=0; k<4; k++)
            {
                for(l=0; l<4; l++)
                {
                    int n = l + k * 4 + j * 16 + i * 80;
                    n_slen2[n] = i|(j<<3)|(k<<6)|(l<<9)|(0<<12);
                }
            }
        }
    }
    for(i=0; i<5; i++)
    {
        for(j=0; j<5; j++)
        {
            for(k=0; k<4; k++)
            {
                int n = k + j * 4 + i * 20;
                n_slen2[n+400] = i|(j<<3)|(k<<6)|(1<<12);
            }
        }
    }
}

/*
 * read additional side information (for MPEG 1 and MPEG 2)
 */

int __fastcall
mpg123::III_get_side_info(III_sideinfo* si, int ms_stereo)
{
    int ch, gr;
    static const int tabs[2][5] = { { 2, 9, 5, 3, 4 }, { 1, 8, 1, 2, 9 } };
    const int *tab = tabs[fr.lsf];

    si->main_data_begin = SideInfoBuff->getbits(tab[1]);

    if(fr.stereo == 1)
    {
        si->private_bits = SideInfoBuff->getbits_fast(tab[2]);
    }
    else
    {
        si->private_bits = SideInfoBuff->getbits_fast(tab[3]);
    }

    if(!fr.lsf)
    {
        for(ch = 0; ch < fr.stereo; ch++)
        {
            si->ch[ch].gr[0].scfsi = -1;
            si->ch[ch].gr[1].scfsi = SideInfoBuff->getbits_fast(4);
        }
    }

    for(gr = 0; gr < tab[0]; gr++)
    {
        for(ch = 0; ch < fr.stereo; ch++)
        {
            /*register*/ struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);

            gr_info->part2_3_length = SideInfoBuff->getbits(12);
            gr_info->big_values = Min(SideInfoBuff->getbits(9), 288U);
//			if(gr_info->big_values > 288) {
//				gr_info->big_values = 288;
//				errorstring = "big_values too large!";
//			}

            gr_info->pow2gain = gainpow2 + 256 - SideInfoBuff->getbits_fast(8);
            if(ms_stereo) gr_info->pow2gain += 2;
            gr_info->scalefac_compress = SideInfoBuff->getbits(tab[4]);

            if(SideInfoBuff->get1bit())   /* window switch flag  */
            {
//				int i;

#ifdef L3_DEBUG
                if(2*gr_info->big_values > bandInfo[fr.frequency].shortIdx[12])
                    fprintf(stderr,"L3: BigValues too large, doesn't make sense %d %d\n",2*gr_info->big_values,bandInfo[fr.frequency].shortIdx[12]);
#endif

                gr_info->block_type       = SideInfoBuff->getbits_fast(2);
                gr_info->mixed_block_flag = SideInfoBuff->get1bit();
                gr_info->table_select[0]  = SideInfoBuff->getbits_fast(5);
                gr_info->table_select[1]  = SideInfoBuff->getbits_fast(5);
                /*
                 * table_select[2] not needed, because there is no region2,
                 * but to satisfy some verifications tools we set it either.
                 */
                gr_info->table_select[2] = 0;
//				for(i = 0; i < 3; i++)  // !
//					gr_info->full_gain[i] = gr_info->pow2gain + (SideInfoBuff->getbits_fast(3)<<3);  // !

                gr_info->full_gain[0] = gr_info->pow2gain + (SideInfoBuff->getbits_fast(3) << 3);  // !
                gr_info->full_gain[1] = gr_info->pow2gain + (SideInfoBuff->getbits_fast(3) << 3);  // !
                gr_info->full_gain[2] = gr_info->pow2gain + (SideInfoBuff->getbits_fast(3) << 3);  // !

                if(gr_info->block_type == 0)
                {
//					errorstring = "Blocktype == 0 and window-switching == 1 not allowed.";
                    return 0;
                }

#if 0
                /* region_count/start parameters are implicit in this case. */
                if(!fr.lsf || (gr_info->block_type == 2))
                {
//           change for MPEG2.5 8kHz
//           ** original **
//           gr_info->region1start = 36>>1;
//           **************
                    gr_info->region1start = ((fr.frequency == 8) ? 72 : 36) >> 1;
                }
                else
                {
                    /* check this again for 2.5 and sfreq=8 */
//           if(sfreq == 8)
//             gr_info->region1start = 108>>1;
//           else
//             gr_info->region1start = 54>>1;
                    gr_info->region1start = ((fr.frequency == 8) ? 108 : 54) >> 1;
                }
#endif

                if(fr.lsf)
                {
                    gr_info->region1start = (gr_info->block_type == 2) ?
                                            (gr_info->mixed_block_flag ? 48 : 36) : 54;
                    if(fr.frequency != 8) gr_info->region1start >>= 1;
                }
                else
                {
                    gr_info->region1start = 36 >> 1;
                }

                gr_info->region2start = 576 >> 1;
            }
            else
            {
                int /*i,*/r0c, r1c;

#ifdef L3_DEBUG
                if(2*gr_info->big_values > bandInfo[fr.frequency].longIdx[21])
                    fprintf(stderr,"L3: BigValues too large, doesn't make sense %d %d\n",2*gr_info->big_values,bandInfo[fr.frequency].longIdx[21]);
#endif

//				for (i = 0; i < 3; i++)  // !
//					gr_info->table_select[i] = SideInfoBuff->getbits_fast(5);  // !

                gr_info->table_select[0] = SideInfoBuff->getbits_fast(5);  // !
                gr_info->table_select[1] = SideInfoBuff->getbits_fast(5);  // !
                gr_info->table_select[2] = SideInfoBuff->getbits_fast(5);  // !

                r0c = SideInfoBuff->getbits_fast(4);
                r1c = SideInfoBuff->getbits_fast(3);
                gr_info->region1start = bandInfo[fr.frequency].longIdx[r0c + 1] >> 1 ;

                if(r0c + r1c + 2 > 22)
                {
                    gr_info->region2start = 576 >> 1;
                }
                else
                {
                    gr_info->region2start = bandInfo[fr.frequency].longIdx[r0c + 1 + r1c + 1] >> 1;
                }

                gr_info->block_type = 0;
                gr_info->mixed_block_flag = 0;
            }

            if(!fr.lsf) gr_info->preflag = SideInfoBuff->get1bit();
            gr_info->scalefac_scale = SideInfoBuff->get1bit();
            gr_info->count1table_select = SideInfoBuff->get1bit();
        }
    }

    return !0;
}

/*
 * read scalefactors
 */
#ifdef MPEG1
int __fastcall
mpg123::III_get_scale_factors_1(int* scf, gr_info_s* gr_info)
{
    static const unsigned char slen[2][16] =
    {
        {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
        {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
    };
    int numbits;
    int num0 = slen[0][gr_info->scalefac_compress];
    int num1 = slen[1][gr_info->scalefac_compress];
    int remaining = gr_info->part2_3_length;

    if(gr_info->block_type == 2)
    {
        int i;

        numbits = (num0 + num1) * 18;

        if(gr_info->mixed_block_flag)
        {
            if(remaining < (num0 * 8)) return -1;
//			for(i=8;i;i--)  // !
//				*scf++ = ReservoirBuff->getbits_fast(num0);  // !

            *scf++ = ReservoirBuff->getbits_fast(num0);  // 1
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 2
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 3
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 4
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 5
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 6
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 7
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 8

            i = 9;
            numbits -= num0; /* num0 * 17 + num1 * 18 */
        }
        else
        {
            i = 18;
        }

        if(remaining < numbits) return -1;

        for(; i; i--)
        {
            *scf++ = ReservoirBuff->getbits_fast(num0);
        }

//		for(i = 18; i; i--)  // !
//			*scf++ = ReservoirBuff->getbits_fast(num1);  // !

        *scf++ = ReservoirBuff->getbits_fast(num1);  // 1
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 2
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 3
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 4
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 5
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 6
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 7
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 8
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 9
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 10
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 11
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 12
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 13
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 14
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 15
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 16
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 17
        *scf++ = ReservoirBuff->getbits_fast(num1);  // 18

        *scf++ = 0;
        *scf++ = 0;
        *scf = 0; /* short[13][0..2] = 0 */
    }
    else
    {
//		int i;
        int scfsi = gr_info->scfsi;

        if(scfsi < 0)   /* scfsi < 0 => granule == 0 */
        {
            numbits = (num0 + num1) * 10 + num0;
            if(remaining < numbits) return -1;
//			for(i=11;i;i--)  // !
//				*scf++ = ReservoirBuff->getbits_fast(num0);  // !

            *scf++ = ReservoirBuff->getbits_fast(num0);  // 1
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 2
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 3
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 4
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 5
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 6
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 7
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 8
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 9
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 10
            *scf++ = ReservoirBuff->getbits_fast(num0);  // 11

//			for(i=10;i;i--)  // !
//				*scf++ = ReservoirBuff->getbits_fast(num1);  // !

            *scf++ = ReservoirBuff->getbits_fast(num1);  // 1
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 2
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 3
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 4
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 5
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 6
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 7
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 8
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 9
            *scf++ = ReservoirBuff->getbits_fast(num1);  // 10
        }
        else
        {
            numbits = 0;

            if(!(scfsi & 0x8))
            {
                numbits += num0 * 6;
                if(remaining < numbits) return -1;
//				for(i=6;i;i--)  // !
//					*scf++ = ReservoirBuff->getbits_fast(num0);  // !

                *scf++ = ReservoirBuff->getbits_fast(num0);  // 1
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 2
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 3
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 4
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 5
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 6
            }
            else
            {
                scf += 6;
            }

            if(!(scfsi & 0x4))
            {
                numbits += num0 * 5;
                if(remaining < numbits) return -1;
//				for(i=5;i;i--)  // !
//					*scf++ = ReservoirBuff->getbits_fast(num0);  // !

                *scf++ = ReservoirBuff->getbits_fast(num0);  // 1
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 2
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 3
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 4
                *scf++ = ReservoirBuff->getbits_fast(num0);  // 5
            }
            else
            {
                scf += 5;
            }

            if(!(scfsi & 0x2))
            {
                numbits += num1 * 5;
                if(remaining < numbits) return -1;
//				for(i=5;i;i--)  // !
//					*scf++ = ReservoirBuff->getbits_fast(num1);  // !

                *scf++ = ReservoirBuff->getbits_fast(num1);  // 1
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 2
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 3
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 4
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 5
            }
            else
            {
                scf += 5;
            }

            if(!(scfsi & 0x1))
            {
                numbits += num1 * 5;
                if(remaining < numbits) return -1;
//				for(i=5;i;i--)  // !
//					*scf++ = ReservoirBuff->getbits_fast(num1);  // !

                *scf++ = ReservoirBuff->getbits_fast(num1);  // 1
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 2
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 3
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 4
                *scf++ = ReservoirBuff->getbits_fast(num1);  // 5
            }
            else
            {
                scf += 5;
            }
        }

        *scf = 0;  /* no l[21] in original sources */
    }

    return numbits;
}
#endif

int __fastcall
mpg123::III_get_scale_factors_2(int* scf, gr_info_s* gr_info, int i_stereo)
{
    const unsigned char *pnt;
    int i, j, n;
    unsigned int slen;

    static const unsigned char stab[3][6][4] =
    {
        {   { 6, 5, 5,5 }, { 6, 5, 7,3 }, { 11,10,0,0},
            { 7, 7, 7,0 }, { 6, 6, 6,3 }, {  8, 8,5,0}
        },
        {   { 9, 9, 9,9 }, { 9, 9,12,6 }, { 18,18,0,0},
            {12,12,12,0 }, {12, 9, 9,6 }, { 15,12,9,0}
        },
        {   { 6, 9, 9,9 }, { 6, 9,12,6 }, { 15,18,0,0},
            { 6,15,12,0 }, { 6,12, 9,6 }, {  6,18,9,0}
        }
    };

    if(i_stereo)  /* i_stereo AND second channel -> do_layer3() checks this */
    {
        slen = i_slen2[gr_info->scalefac_compress >> 1];
    }
    else
    {
        slen = n_slen2[gr_info->scalefac_compress];
    }

    gr_info->preflag = (slen >> 15) & 0x1;

    n = 0;

    if(gr_info->block_type == 2)
    {
        n++;
        if(gr_info->mixed_block_flag) n++;
    }

    pnt = stab[n][(slen >> 12) & 0x7];

    int remaining = gr_info->part2_3_length;

    for(i = 0; i < 4; i++)
    {
        int num = slen & 0x7;

        slen >>= 3;

        if(num)
        {
            remaining -= num * pnt[i];
            if(remaining < 0) return -1;

            for(j = 0; j < (int)(pnt[i]); j++)
            {
                *scf++ = ReservoirBuff->getbits_fast(num);
            }
        }
        else
        {
//			for(j=0;j<(int)(pnt[i]);j++)  // !
//			*scf++ = 0;  //!
            memset(scf, 0, pnt[i] * sizeof(int));  // !
            scf += pnt[i];  // !
        }
    }

    n = (n << 1) + 1;

//	for(i = 0; i < n; i++) *scf++ = 0;  // !
    memset(scf, 0, n * sizeof(int));  // !

    return gr_info->part2_3_length - remaining;
}

static const int pretab1[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
static const int pretab2[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*
 * Dequantize samples (includes huffman decoding)
 */
/* 24 is enough because tab13 has max. a 19 bit huffvector */

int __fastcall
mpg123::III_dequantize_sample(
    real xr[SBLIMIT][SSLIMIT],
    int* scf,
    gr_info_s* gr_info,
    int part2bits)
{
    int part2remain = gr_info->part2_3_length - part2bits;

    if((part2bits < 0) || (part2remain == 0))
    {
        gr_info->maxband[0] = 0;
        gr_info->maxband[1] = 0;
        gr_info->maxband[2] = 0;
        gr_info->maxbandl = 0;
        gr_info->maxb = 1;
        memset(xr, 0, SBLIMIT * SSLIMIT * sizeof(real));
        return 0;
    }

    if(part2remain < 0) return 1;

    int shift = 1 + gr_info->scalefac_scale;
    real *xrpnt = (real*)xr;
    int l[3], l3;
    int *me;

    int bv      = gr_info->big_values;
    int region1 = gr_info->region1start;
    int region2 = gr_info->region2start;

    if(region1 > region2) return 1;

    l3 = ((576 >> 1) - bv) >> 1;

    /*
     * we may lose the 'odd' bit here !!
     * check this later again
     */

    if(bv <= region1)
    {
        l[0] = bv;
        l[1] = l[2] = 0;
    }
    else
    {
        l[0] = region1;

        if(bv <= region2)
        {
            l[1] = Max(bv - l[0], 0);
            l[2] = 0;
        }
        else
        {
            l[1] = Max(region2 - l[0], 0);
            l[2] = bv - region2;
        }
    }

    ReservoirBuff->Cache_Start();

    if(gr_info->block_type == 2)
    {
        /*
         * decoding with short or mixed mode BandIndex table
         */
        int i, max[4];
        int step = 0, lwin = 3, cb = 0;  // !
        /*register*/ real v = 0.0;
        /*register*/ int *m, mc;

        if(gr_info->mixed_block_flag)
        {
            max[3] = -1;
            max[0] = max[1] = max[2] = 2;
            m = map[fr.frequency][0];
            me = mapend[fr.frequency][0];
        }
        else
        {
            max[0] = max[1] = max[2] = max[3] = -1;
            /* max[3] not really needed in this case */
            m = map[fr.frequency][1];
            me = mapend[fr.frequency][1];
        }

        mc = 0;

        for(i = 0; i < 2; i++)
        {
            int lp = l[i];
            struct newhuff *h = ht + gr_info->table_select[i];

            for(; lp; lp--, mc--)
            {
                /*register*/ int x,y;

                if(!mc)
                {
                    mc = *m++;
                    xrpnt = ((real*)xr) + (*m++);
                    lwin = *m++;
                    cb = *m++;

                    if(lwin == 3)
                    {
                        v = gr_info->pow2gain[(*scf++) << shift];
                        step = 1;
                    }
                    else
                    {
                        v = gr_info->full_gain[lwin][(*scf++) << shift];
                        step = 3;
                    }
                }

                /*register*/ short *val = h->table;

                ReservoirBuff->Cache_Charge();

                while((y = *val++) < 0)
                {
                    if(part2remain == 0) break;
                    if(ReservoirBuff->Cache_get1bit()) val -= y;
                    part2remain--;
                }

                if(part2remain == 0) break;
                x = y >> 4;
                y &= 0xf;

                if(x == 15 && h->linbits)
                {
                    max[lwin] = cb;
                    if(part2remain < (int)(h->linbits + 1)) break;
                    ReservoirBuff->Cache_Charge();
                    x += ReservoirBuff->Cache_getbits_fast(h->linbits);

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt = -ispow[x] * v;
                    }
                    else
                    {
                        *xrpnt =  ispow[x] * v;
                    }

                    part2remain -= h->linbits+1;
                }
                else if(x)
                {
                    max[lwin] = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt = -ispow[x] * v;
                    }
                    else
                    {
                        *xrpnt =  ispow[x] * v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt = 0.0;
                }

                xrpnt += step;

                if(y == 15 && h->linbits)
                {
                    max[lwin] = cb;
                    if(part2remain < (int)(h->linbits + 1)) break;
                    ReservoirBuff->Cache_Charge();
                    y += ReservoirBuff->Cache_getbits_fast(h->linbits);

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt = -ispow[y] * v;
                    }
                    else
                    {
                        *xrpnt =  ispow[y] * v;
                    }

                    part2remain -= h->linbits+1;
                }
                else if(y)
                {
                    max[lwin] = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt = -ispow[y] * v;
                    }
                    else
                    {
                        *xrpnt =  ispow[y] * v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt = 0.0;
                }

                xrpnt += step;
            }
        }

        for(; l3 && (part2remain > 0); l3--)
        {
            if(!(xrpnt < &xr[SBLIMIT][0] + 5)) return 1;

            struct newhuff *h = htc + gr_info->count1table_select;
            /*register*/ short *val = h->table,a;

            ReservoirBuff->Cache_Charge();

            while((a = *val++) < 0)
            {
                if(part2remain == 0) break;
                if(ReservoirBuff->Cache_get1bit()) val -= a;
                part2remain--;
            }

            if(part2remain == 0) break;

            for(i = 0; i < 4; i++)
            {
                if(!(i & 1))
                {
                    if(!mc)
                    {
                        mc = *m++;
                        xrpnt = ((real*)xr) + (*m++);
                        lwin = *m++;
                        cb = *m++;

                        if(lwin == 3)
                        {
                            v = gr_info->pow2gain[(*scf++) << shift];
                            step = 1;
                        }
                        else
                        {
                            v = gr_info->full_gain[lwin][(*scf++) << shift];
                            step = 3;
                        }
                    }

                    mc--;
                }

                if((a & (0x8 >> i)))
                {
                    max[lwin] = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt = -v;
                    }
                    else
                    {
                        *xrpnt = v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt = 0.0;
                }

                xrpnt += step;
            }
        }

        if(lwin < 3)   /* short band? */
        {
            while(1)
            {
                for(; mc > 0; mc--)
                {
                    *xrpnt = 0.0;
                    xrpnt += 3; /* short band -> step=3 */
                    *xrpnt = 0.0;
                    xrpnt += 3;
                }

                if(m >= me) break;
                mc = *m++;
                xrpnt = ((real*)xr) + *m++;

                if(*m++ == 0) break; /* optimize: field will be set to zero at the end of the function */
                m++; /* cb */
            }
        }

//
//    while( m < me ) {
//      if(!mc) {
//        mc = *m++;
//        xrpnt = ((real *) xr) + *m++;
//       if( (*m++) == 3)
//          step = 1;
//       else
//          step = 3;
//        m++; /* cb */
//      }
//      mc--;
//      *xrpnt = 0.0;
//      xrpnt += step;
//      *xrpnt = 0.0;
//      xrpnt += step;

        /* we could add a little opt. here:
         * if we finished a band for window 3 or a long band
         * further bands could copied in a simple loop without a
         * special 'map' decoding
         */
//    }

        gr_info->maxband[0] = max[0] + 1;
        gr_info->maxband[1] = max[1] + 1;
        gr_info->maxband[2] = max[2] + 1;
        gr_info->maxbandl = max[3] + 1;

        int rmax = Max(max[0], max[1]);

        rmax = Max(rmax, max[2]) + 1;
        gr_info->maxb = rmax ? shortLimit[fr.frequency][rmax] : longLimit[fr.frequency][max[3] + 1];
    }
    else
    {
        /*
         * decoding with 'long' BandIndex table (block_type != 2)
         */
        const int *pretab = gr_info->preflag ? pretab1 : pretab2;
        int i, max = -1;
        int cb = 0;
        int *m = map[fr.frequency][2];
        /*register*/ real v = 0.0;
        int mc = 0;

        /*
         * long hash table values
         */
        for(i = 0; i < 3; i++)
        {
            int lp = l[i];
            struct newhuff *h = ht + gr_info->table_select[i];

            for(; lp; lp--, mc--)
            {
                int x, y;

                if(!mc)
                {
                    mc = *m++;
                    cb = *m++;
                    v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
                }

                /*register*/ short *val = h->table;

                ReservoirBuff->Cache_Charge();

                while((y = *val++) < 0)
                {
                    if(part2remain == 0) break;
                    if(ReservoirBuff->Cache_get1bit()) val -= y;
                    part2remain--;
                }

                if(part2remain == 0) break;
                x = y >> 4;
                y &= 0xf;

                if(x == 15 && h->linbits)
                {
                    max = cb;
                    if(part2remain < (int)(h->linbits + 1)) break;
                    ReservoirBuff->Cache_Charge();
                    x += ReservoirBuff->Cache_getbits_fast(h->linbits);

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt++ = -ispow[x] * v;
                    }
                    else
                    {
                        *xrpnt++ =  ispow[x] * v;
                    }

                    part2remain -= h->linbits+1;
                }
                else if(x)
                {
                    max = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt++ = -ispow[x] * v;
                    }
                    else
                    {
                        *xrpnt++ =  ispow[x] * v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt++ = 0.0;
                }

                if(y == 15 && h->linbits)
                {
                    max = cb;
                    if(part2remain < (int)(h->linbits + 1)) break;
                    ReservoirBuff->Cache_Charge();
                    y += ReservoirBuff->Cache_getbits_fast(h->linbits);

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt++ = -ispow[y] * v;
                    }
                    else
                    {
                        *xrpnt++ =  ispow[y] * v;
                    }

                    part2remain -= h->linbits+1;
                }
                else if(y)
                {
                    max = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt++ = -ispow[y] * v;
                    }
                    else
                    {
                        *xrpnt++ =  ispow[y] * v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt++ = 0.0;
                }
            }
        }

        /*
         * short (count1table) values
         */
        for(; l3 && (part2remain > 0); l3--)
        {
            struct newhuff *h = htc + gr_info->count1table_select;
            /*register*/ short *val = h->table,a;

            ReservoirBuff->Cache_Charge();

            while((a = *val++) < 0)
            {
                if(part2remain == 0) break;
                if(ReservoirBuff->Cache_get1bit()) val -= a;
                part2remain--;
            }

            if(part2remain == 0) break;

            for(i = 0; i < 4; i++)
            {
                if(!(i & 1))
                {
                    if(!mc)
                    {
                        mc = *m++;
                        cb = *m++;
                        v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
                    }

                    mc--;
                }

                if(a & (0x8 >> i))
                {
                    max = cb;
                    if(part2remain == 0) break;

                    if(ReservoirBuff->Cache_get1bit())
                    {
                        *xrpnt++ = -v;
                    }
                    else
                    {
                        *xrpnt++ = v;
                    }

                    part2remain--;
                }
                else
                {
                    *xrpnt++ = 0.0;
                }
            }
        }

        gr_info->maxbandl = max + 1;
        gr_info->maxb = longLimit[fr.frequency][gr_info->maxbandl];
    }

    ReservoirBuff->Cache_End();

//	while(xrpnt < &xr[SBLIMIT][0]) *xrpnt++ = 0.0;  // !
    if(xrpnt < &xr[SBLIMIT][0]) memset(xrpnt, 0, (&xr[SBLIMIT][0] - xrpnt) * sizeof(real));  // !

    while(part2remain > 16)
    {
        ReservoirBuff->getbits(16); /* Dismiss stuffing Bits */
        part2remain -= 16;
    }

    if(part2remain > 0)
    {
        ReservoirBuff->getbits(part2remain);
    }
    else if(part2remain < 0)
    {
//		errorstring = "mpg123: Can't rewind stream by %d bits!";
        return 1; /* -> error */
    }

    return 0;
}

/*
 * III_stereo: calculate real channel values for Joint-I-Stereo-mode
 */

void __fastcall
mpg123::III_i_stereo(
    real xr_buf[2][SBLIMIT][SSLIMIT],
    int* scalefac,
    gr_info_s* gr_info,
    int ms_stereo)
{
    real (*xr)[SBLIMIT*SSLIMIT] = (real (*)[SBLIMIT*SSLIMIT] ) xr_buf;
    const struct bandInfoStruct *bi = &bandInfo[fr.frequency];
    const real *tab1, *tab2;

    int tab;
    static const real *tabs[3][2][2] =
    {
        { { tan1_1, tan2_1 }, { tan1_2, tan2_2 } },
        { { pow1_1[0], pow2_1[0] }, { pow1_2[0], pow2_2[0] } },
        { { pow1_1[1], pow2_1[1] }, { pow1_2[1], pow2_2[1] } }
    };

    tab = fr.lsf + (gr_info->scalefac_compress & fr.lsf);
    tab1 = tabs[tab][ms_stereo][0];
    tab2 = tabs[tab][ms_stereo][1];
#if 0
    if(fr.lsf)
    {
        int p = gr_info->scalefac_compress & 0x1;
        if(ms_stereo)
        {
            tab1 = pow1_2[p];
            tab2 = pow2_2[p];
        }
        else
        {
            tab1 = pow1_1[p];
            tab2 = pow2_1[p];
        }
    }
    else
    {
        if(ms_stereo)
        {
            tab1 = tan1_2;
            tab2 = tan2_2;
        }
        else
        {
            tab1 = tan1_1;
            tab2 = tan2_1;
        }
    }
#endif

    if(gr_info->block_type == 2)
    {
        int lwin, do_l = 0;

        if(gr_info->mixed_block_flag) do_l = 1;

        for(lwin = 0; lwin < 3; lwin++)   /* process each window */
        {
            /* get first band with zero values */
            int is_p, sb, idx, sfb = gr_info->maxband[lwin]; /* sfb is minimal 3 for mixed mode */

            if(sfb > 3) do_l = 0;

            for(; sfb < 12; sfb++)
            {
                is_p = scalefac[sfb * 3 + lwin - gr_info->mixed_block_flag]; /* scale: 0-15 */

                if((is_p >= 0) && (is_p < 32) && (is_p != 7))    // !
                {
                    real t1, t2;

                    sb = bi->shortDiff[sfb];
                    idx = bi->shortIdx[sfb] + lwin;
                    if(idx < 0) return;  // !
                    t1 = tab1[is_p];
                    t2 = tab2[is_p];

                    for(; sb > 0; sb--, idx += 3)
                    {
                        if(idx >= (SBLIMIT * SSLIMIT)) return;  // !

                        real v = xr[0][idx];

                        xr[0][idx] = v * t1;
                        xr[1][idx] = v * t2;
                    }
                }
            }

            /* in the original: copy 10 to 11 , here: copy 11 to 12
            maybe still wrong??? (copy 12 to 13?) */
            is_p = scalefac[11 * 3 + lwin - gr_info->mixed_block_flag]; /* scale: 0-15 */

            if((is_p >= 0) && (is_p < 32) && (is_p != 7))    // !
            {
                real t1, t2;

                sb = bi->shortDiff[12];
                idx = bi->shortIdx[12] + lwin;
                if(idx < 0) return;  // !
                t1 = tab1[is_p];
                t2 = tab2[is_p];

                for(; sb > 0; sb--, idx += 3)
                {
                    if(idx >= (SBLIMIT * SSLIMIT)) return;  // !

                    real v = xr[0][idx];

                    xr[0][idx] = v * t1;
                    xr[1][idx] = v * t2;
                }
            }
        } /* end for(lwin; .. ; . ) */

        /* also check l-part, if ALL bands in the three windows are 'empty'
         * and mode = mixed_mode
         */

        if(do_l)
        {
            int sfb = gr_info->maxbandl;
            int idx = bi->longIdx[sfb];

            if(idx < 0) return;  // !

            for(; sfb < 8; sfb++)
            {
                int sb = bi->longDiff[sfb];
                int is_p = scalefac[sfb]; /* scale: 0-15 */

                if((is_p >= 0) && (is_p != 7))    // !
                {
                    real t1, t2;

                    t1 = tab1[is_p];
                    t2 = tab2[is_p];

                    for(; sb > 0; sb--, idx++)
                    {
                        if(idx >= (SBLIMIT * SSLIMIT)) return;  // !

                        real v = xr[0][idx];

                        xr[0][idx] = v * t1;
                        xr[1][idx] = v * t2;
                    }
                }
                else
                {
                    idx += sb;
                }
            }
        }
    }
    else /* ((gr_info->block_type != 2)) */
    {
        int sfb = gr_info->maxbandl;

        /* hmm ... maybe the maxbandl stuff for i-stereo is buggy? */
        if(sfb <= 21)
        {
            int is_p;
            int idx = bi->longIdx[sfb];

            if(idx < 0) return;  // !

            for(; sfb < 21; sfb++)
            {
                int sb = bi->longDiff[sfb];

                is_p = scalefac[sfb]; /* scale: 0-15 */

                if((is_p >= 0) && (is_p < 32) && (is_p != 7))    // !
                {
                    real t1, t2;

                    t1 = tab1[is_p];
                    t2 = tab2[is_p];

                    for(; sb > 0; sb--, idx++)
                    {
                        if(idx >= (SBLIMIT * SSLIMIT)) return;  // !

                        real v = xr[0][idx];

                        xr[0][idx] = v * t1;
                        xr[1][idx] = v * t2;
                    }
                }
                else
                {
                    idx += sb;
                }
            }

            is_p = scalefac[20]; /* copy l-band 20 to l-band 21 */

            if((is_p >= 0) && (is_p < 32) && (is_p != 7))    // !
            {
                int sb;
                real t1, t2;

                t1 = tab1[is_p];
                t2 = tab2[is_p];

                for(sb = bi->longDiff[21]; sb > 0; sb--, idx++)
                {
                    if(idx >= (SBLIMIT * SSLIMIT)) return;  // !

                    real v = xr[0][idx];

                    xr[0][idx] = v * t1;
                    xr[1][idx] = v * t2;
                }
            }
        } /* end: if(sfb <= 21) */
    } /* ... */
}

void __fastcall
mpg123::III_antialias(real xr[SBLIMIT][SSLIMIT], gr_info_s* gr_info)
{
    unsigned int sblim;

    if(gr_info->block_type == 2)
    {
        if(!gr_info->mixed_block_flag) return;
        sblim = 1;
    }
    else
    {
        sblim = gr_info->maxb - 1;
    }

    /* 31 alias-reduction operations between each pair of sub-bands */
    /* with 8 butterflies between each pair                         */

    unsigned int sb;
    real *xr1 = (real*)xr[1];

    for(sb = sblim; sb > 0; sb--, xr1 += 10)
    {
//		int ss;
        real *cs = aa_cs, *ca = aa_ca;
        real *xr2 = xr1;

//		for(ss=7;ss>=0;ss--)  // !
//		{       /* upper and lower butterfly inputs */  // !
//			/*register*/ real bu = *--xr2,bd = *xr1;  // !
//			*xr2   = (bu * (*cs)   ) - (bd * (*ca)   );  // !
//			*xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );  // !
//		}  // !

        /*register*/ real bu;
        /*register*/ real bd;

        // 1

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 2

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 3

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 4

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 5

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 6

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 7

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );

        // 8

        bu = *--xr2;
        bd = *xr1;
        *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );
    }
}

/*
 This is an optimized DCT from Jeff Tsay's maplay 1.2+ package.
 Saved one multiplication by doing the 'twiddle factor' stuff
 together with the window mul. (MH)

 This uses Byeong Gi Lee's Fast Cosine Transform algorithm, but the
 9 point IDCT needs to be reduced further. Unfortunately, I don't
 know how to do that, because 9 is not an even number. - Jeff.

 ****************************************************************

 9 Point Inverse Discrete Cosine Transform

 This piece of code is Copyright 1997 Mikko Tommila and is freely usable
 by anybody. The algorithm itself is of course in the public domain.

 Again derived heuristically from the 9-point WFTA.

 The algorithm is optimized (?) for speed, not for small rounding errors or
 good readability.

 36 additions, 11 multiplications

 Again this is very likely sub-optimal.

 The code is optimized to use a minimum number of temporary variables,
 so it should compile quite well even on 8-register Intel x86 processors.
 This makes the code quite obfuscated and very difficult to understand.

 References:
 [1] S. Winograd: "On Computing the Discrete Fourier Transform",
     Mathematics of Computation, Volume 32, Number 141, January 1978,
     Pages 175-199
*/

/*------------------------------------------------------------------*/
/*                                                                  */
/*    Function: Calculation of the inverse MDCT                     */
/*                                                                  */
/*------------------------------------------------------------------*/

static void __fastcall dct36(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf)
{
#ifdef NEW_DCT9
    real tmp[18];
#endif

    {
        /*register*/ real *in = inbuf;

        in[17]+=in[16];
        in[16]+=in[15];
        in[15]+=in[14];
        in[14]+=in[13];
        in[13]+=in[12];
        in[12]+=in[11];
        in[11]+=in[10];
        in[10]+=in[9];
        in[9] +=in[8];
        in[8] +=in[7];
        in[7] +=in[6];
        in[6] +=in[5];
        in[5] +=in[4];
        in[4] +=in[3];
        in[3] +=in[2];
        in[2] +=in[1];
        in[1] +=in[0];

        in[17]+=in[15];
        in[15]+=in[13];
        in[13]+=in[11];
        in[11]+=in[9];
        in[9] +=in[7];
        in[7] +=in[5];
        in[5] +=in[3];
        in[3] +=in[1];


#ifdef NEW_DCT9
#if 1
        {
            real t3;
            {
                real t0, t1, t2;

                t0 = COS6_2 * (in[8] + in[16] - in[4]);
                t1 = COS6_2 * in[12];

                t3 = in[0];
                t2 = t3 - t1 - t1;
                tmp[1] = tmp[7] = t2 - t0;
                tmp[4]          = t2 + t0 + t0;
                t3 += t1;

                t2 = COS6_1 * (in[10] + in[14] - in[2]);
                tmp[1] -= t2;
                tmp[7] += t2;
            }
            {
                real t0, t1, t2;

                t0 = cos9[0] * (in[4] + in[8] );
                t1 = cos9[1] * (in[8] - in[16]);
                t2 = cos9[2] * (in[4] + in[16]);

                tmp[2] = tmp[6] = t3 - t0      - t2;
                tmp[0] = tmp[8] = t3 + t0 + t1;
                tmp[3] = tmp[5] = t3      - t1 + t2;
            }
        }
        {
            real t1, t2, t3;

            t1 = cos18[0] * (in[2]  + in[10]);
            t2 = cos18[1] * (in[10] - in[14]);
            t3 = COS6_1 * in[6];

            {
                real t0 = t1 + t2 + t3;
                tmp[0] += t0;
                tmp[8] -= t0;
            }

            t2 -= t3;
            t1 -= t3;

            t3 = cos18[2] * (in[2] + in[14]);

            t1 += t3;
            tmp[3] += t1;
            tmp[5] -= t1;

            t2 -= t3;
            tmp[2] += t2;
            tmp[6] -= t2;
        }

#else
        {
            real t0, t1, t2, t3, t4, t5, t6, t7;

            t1 = COS6_2 * in[12];
            t2 = COS6_2 * (in[8] + in[16] - in[4]);

            t3 = in[0] + t1;
            t4 = in[0] - t1 - t1;
            t5     = t4 - t2;
            tmp[4] = t4 + t2 + t2;

            t0 = cos9[0] * (in[4] + in[8]);
            t1 = cos9[1] * (in[8] - in[16]);

            t2 = cos9[2] * (in[4] + in[16]);

            t6 = t3 - t0 - t2;
            t0 += t3 + t1;
            t3 += t2 - t1;

            t2 = cos18[0] * (in[2]  + in[10]);
            t4 = cos18[1] * (in[10] - in[14]);
            t7 = COS6_1 * in[6];

            t1 = t2 + t4 + t7;
            tmp[0] = t0 + t1;
            tmp[8] = t0 - t1;
            t1 = cos18[2] * (in[2] + in[14]);
            t2 += t1 - t7;

            tmp[3] = t3 + t2;
            t0 = COS6_1 * (in[10] + in[14] - in[2]);
            tmp[5] = t3 - t2;

            t4 -= t1 + t7;

            tmp[1] = t5 - t0;
            tmp[7] = t5 + t0;
            tmp[2] = t6 + t4;
            tmp[6] = t6 - t4;
        }
#endif

        {
            real t0, t1, t2, t3, t4, t5, t6, t7;

            t1 = COS6_2 * in[13];
            t2 = COS6_2 * (in[9] + in[17] - in[5]);

            t3 = in[1] + t1;
            t4 = in[1] - t1 - t1;
            t5 = t4 - t2;

            t0 = cos9[0] * (in[5] + in[9]);
            t1 = cos9[1] * (in[9] - in[17]);

            tmp[13] = (t4 + t2 + t2) * tfcos36[17-13];
            t2 = cos9[2] * (in[5] + in[17]);

            t6 = t3 - t0 - t2;
            t0 += t3 + t1;
            t3 += t2 - t1;

            t2 = cos18[0] * (in[3]  + in[11]);
            t4 = cos18[1] * (in[11] - in[15]);
            t7 = COS6_1 * in[7];

            t1 = t2 + t4 + t7;
            tmp[17] = (t0 + t1) * tfcos36[17-17];
            tmp[9]  = (t0 - t1) * tfcos36[17-9];
            t1 = cos18[2] * (in[3] + in[15]);
            t2 += t1 - t7;

            tmp[14] = (t3 + t2) * tfcos36[17-14];
            t0 = COS6_1 * (in[11] + in[15] - in[3]);
            tmp[12] = (t3 - t2) * tfcos36[17-12];

            t4 -= t1 + t7;

            tmp[16] = (t5 - t0) * tfcos36[17-16];
            tmp[10] = (t5 + t0) * tfcos36[17-10];
            tmp[15] = (t6 + t4) * tfcos36[17-15];
            tmp[11] = (t6 - t4) * tfcos36[17-11];
        }

#define MACRO(v) { \
    real tmpval; \
    tmpval = tmp[(v)] + tmp[17-(v)]; \
    out2[9+(v)] = (tmpval * w[27+(v)]); \
    out2[8-(v)] = (tmpval * w[26-(v)]); \
    tmpval = tmp[(v)] - tmp[17-(v)]; \
    ts[SBLIMIT*(8-(v))] = out1[8-(v)] + (tmpval * w[8-(v)]); \
    ts[SBLIMIT*(9+(v))] = out1[9+(v)] + (tmpval * w[9+(v)]); }

        {
            /*register*/ real *out2 = o2;
            /*register*/ real *w = wintab;
            /*register*/ real *out1 = o1;
            /*register*/ real *ts = tsbuf;

            MACRO(0);
            MACRO(1);
            MACRO(2);
            MACRO(3);
            MACRO(4);
            MACRO(5);
            MACRO(6);
            MACRO(7);
            MACRO(8);
        }

#else

        {

#define MACRO0(v) { \
    real tmp; \
    out2[9+(v)] = (tmp = sum0 + sum1) * w[27+(v)]; \
    out2[8-(v)] = tmp * w[26-(v)];  } \
    sum0 -= sum1; \
    ts[SBLIMIT*(8-(v))] = out1[8-(v)] + sum0 * w[8-(v)]; \
    ts[SBLIMIT*(9+(v))] = out1[9+(v)] + sum0 * w[9+(v)];
#define MACRO1(v) { \
	real sum0,sum1; \
    sum0 = tmp1a + tmp2a; \
	sum1 = (tmp1b + tmp2b) * tfcos36[(v)]; \
	MACRO0(v); }
#define MACRO2(v) { \
    real sum0,sum1; \
    sum0 = tmp2a - tmp1a; \
    sum1 = (tmp2b - tmp1b) * tfcos36[(v)]; \
	MACRO0(v); }

            /*register*/ const real *c = COS9;
            /*register*/ real *out2 = o2;
            /*register*/ real *w = wintab;
            /*register*/ real *out1 = o1;
            /*register*/ real *ts = tsbuf;

            real ta33,ta66,tb33,tb66;

            ta33 = in[2*3+0] * c[3];
            ta66 = in[2*6+0] * c[6];
            tb33 = in[2*3+1] * c[3];
            tb66 = in[2*6+1] * c[6];

            {
                real tmp1a,tmp2a,tmp1b,tmp2b;
                tmp1a =             in[2*1+0] * c[1] + ta33 + in[2*5+0] * c[5] + in[2*7+0] * c[7];
                tmp1b =             in[2*1+1] * c[1] + tb33 + in[2*5+1] * c[5] + in[2*7+1] * c[7];
                tmp2a = in[2*0+0] + in[2*2+0] * c[2] + in[2*4+0] * c[4] + ta66 + in[2*8+0] * c[8];
                tmp2b = in[2*0+1] + in[2*2+1] * c[2] + in[2*4+1] * c[4] + tb66 + in[2*8+1] * c[8];

                MACRO1(0);
                MACRO2(8);
            }

            {
                real tmp1a,tmp2a,tmp1b,tmp2b;
                tmp1a = ( in[2*1+0] - in[2*5+0] - in[2*7+0] ) * c[3];
                tmp1b = ( in[2*1+1] - in[2*5+1] - in[2*7+1] ) * c[3];
                tmp2a = ( in[2*2+0] - in[2*4+0] - in[2*8+0] ) * c[6] - in[2*6+0] + in[2*0+0];
                tmp2b = ( in[2*2+1] - in[2*4+1] - in[2*8+1] ) * c[6] - in[2*6+1] + in[2*0+1];

                MACRO1(1);
                MACRO2(7);
            }

            {
                real tmp1a,tmp2a,tmp1b,tmp2b;
                tmp1a =             in[2*1+0] * c[5] - ta33 - in[2*5+0] * c[7] + in[2*7+0] * c[1];
                tmp1b =             in[2*1+1] * c[5] - tb33 - in[2*5+1] * c[7] + in[2*7+1] * c[1];
                tmp2a = in[2*0+0] - in[2*2+0] * c[8] - in[2*4+0] * c[2] + ta66 + in[2*8+0] * c[4];
                tmp2b = in[2*0+1] - in[2*2+1] * c[8] - in[2*4+1] * c[2] + tb66 + in[2*8+1] * c[4];

                MACRO1(2);
                MACRO2(6);
            }

            {
                real tmp1a,tmp2a,tmp1b,tmp2b;
                tmp1a =             in[2*1+0] * c[7] - ta33 + in[2*5+0] * c[1] - in[2*7+0] * c[5];
                tmp1b =             in[2*1+1] * c[7] - tb33 + in[2*5+1] * c[1] - in[2*7+1] * c[5];
                tmp2a = in[2*0+0] - in[2*2+0] * c[4] + in[2*4+0] * c[8] + ta66 - in[2*8+0] * c[2];
                tmp2b = in[2*0+1] - in[2*2+1] * c[4] + in[2*4+1] * c[8] + tb66 - in[2*8+1] * c[2];

                MACRO1(3);
                MACRO2(5);
            }

            {
                real sum0,sum1;
                sum0 =  in[2*0+0] - in[2*2+0] + in[2*4+0] - in[2*6+0] + in[2*8+0];
                sum1 = (in[2*0+1] - in[2*2+1] + in[2*4+1] - in[2*6+1] + in[2*8+1] ) * tfcos36[4];
                MACRO0(4);
            }
        }
#endif

    }
}

/*
 * new DCT12
 */
static void __fastcall dct12(real *in,real *rawout1,real *rawout2,/*register*/ real *wi,/*register*/ real *ts)
{
#define DCT12_PART1 \
             in5 = in[5*3];  \
     in5 += (in4 = in[4*3]); \
     in4 += (in3 = in[3*3]); \
     in3 += (in2 = in[2*3]); \
     in2 += (in1 = in[1*3]); \
     in1 += (in0 = in[0*3]); \
                             \
     in5 += in3; in3 += in1; \
                             \
     in2 *= COS6_1; \
     in3 *= COS6_1; \

#define DCT12_PART2 \
     in0 += in4 * COS6_2; \
                          \
     in4 = in0 + in2;     \
     in0 -= in2;          \
                          \
     in1 += in5 * COS6_2; \
                          \
     in5 = (in1 + in3) * tfcos12[0]; \
     in1 = (in1 - in3) * tfcos12[2]; \
                         \
     in3 = in4 + in5;    \
     in4 -= in5;         \
                         \
     in2 = in0 + in1;    \
     in0 -= in1;


    {
        real in0,in1,in2,in3,in4,in5;
        /*register*/ real *out1 = rawout1;
        ts[SBLIMIT*0] = out1[0];
        ts[SBLIMIT*1] = out1[1];
        ts[SBLIMIT*2] = out1[2];
        ts[SBLIMIT*3] = out1[3];
        ts[SBLIMIT*4] = out1[4];
        ts[SBLIMIT*5] = out1[5];

        DCT12_PART1

        {
            real tmp0,tmp1 = (in0 - in4);
            {
                real tmp2 = (in1 - in5) * tfcos12[1];
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }
            ts[(17-1)*SBLIMIT] = out1[17-1] + tmp0 * wi[11-1];
            ts[(12+1)*SBLIMIT] = out1[12+1] + tmp0 * wi[6+1];
            ts[(6 +1)*SBLIMIT] = out1[6 +1] + tmp1 * wi[1];
            ts[(11-1)*SBLIMIT] = out1[11-1] + tmp1 * wi[5-1];
        }

        DCT12_PART2

        ts[(17-0)*SBLIMIT] = out1[17-0] + in2 * wi[11-0];
        ts[(12+0)*SBLIMIT] = out1[12+0] + in2 * wi[6+0];
        ts[(12+2)*SBLIMIT] = out1[12+2] + in3 * wi[6+2];
        ts[(17-2)*SBLIMIT] = out1[17-2] + in3 * wi[11-2];

        ts[(6+0)*SBLIMIT]  = out1[6+0] + in0 * wi[0];
        ts[(11-0)*SBLIMIT] = out1[11-0] + in0 * wi[5-0];
        ts[(6+2)*SBLIMIT]  = out1[6+2] + in4 * wi[2];
        ts[(11-2)*SBLIMIT] = out1[11-2] + in4 * wi[5-2];
    }

    in++;

    {
        real in0,in1,in2,in3,in4,in5;
        /*register*/ real *out2 = rawout2;

        DCT12_PART1

        {
            real tmp0,tmp1 = (in0 - in4);
            {
                real tmp2 = (in1 - in5) * tfcos12[1];
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }
            out2[5-1] = tmp0 * wi[11-1];
            out2[0+1] = tmp0 * wi[6+1];
            ts[(12+1)*SBLIMIT] += tmp1 * wi[1];
            ts[(17-1)*SBLIMIT] += tmp1 * wi[5-1];
        }

        DCT12_PART2

        out2[5-0] = in2 * wi[11-0];
        out2[0+0] = in2 * wi[6+0];
        out2[0+2] = in3 * wi[6+2];
        out2[5-2] = in3 * wi[11-2];

        ts[(12+0)*SBLIMIT] += in0 * wi[0];
        ts[(17-0)*SBLIMIT] += in0 * wi[5-0];
        ts[(12+2)*SBLIMIT] += in4 * wi[2];
        ts[(17-2)*SBLIMIT] += in4 * wi[5-2];
    }

    in++;

    {
        real in0,in1,in2,in3,in4,in5;
        /*register*/ real *out2 = rawout2;
        out2[12]=out2[13]=out2[14]=out2[15]=out2[16]=out2[17]=0.0;

        DCT12_PART1

        {
            real tmp0,tmp1 = (in0 - in4);
            {
                real tmp2 = (in1 - in5) * tfcos12[1];
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }
            out2[11-1] = tmp0 * wi[11-1];
            out2[6 +1] = tmp0 * wi[6+1];
            out2[0+1] += tmp1 * wi[1];
            out2[5-1] += tmp1 * wi[5-1];
        }

        DCT12_PART2

        out2[11-0] = in2 * wi[11-0];
        out2[6 +0] = in2 * wi[6+0];
        out2[6 +2] = in3 * wi[6+2];
        out2[11-2] = in3 * wi[11-2];

        out2[0+0] += in0 * wi[0];
        out2[5-0] += in0 * wi[5-0];
        out2[0+2] += in4 * wi[2];
        out2[5-2] += in4 * wi[5-2];
    }
}

/*
 * III_hybrid
 */

void __fastcall mpg123::III_hybrid(
    real fsIn[SBLIMIT][SSLIMIT],
    real tsOut[SSLIMIT][SBLIMIT],
    int ch,
    gr_info_s* gr_info)
{
    real *tspnt = (real*)tsOut;
    real *rawout1, *rawout2;
    int bt;
    unsigned int sb;

    int b = hybrid_blc[ch];

    rawout1 = hybrid_block[b][ch];
    b = -b + 1;
    rawout2 = hybrid_block[b][ch];
    hybrid_blc[ch] = b;

    if(gr_info->mixed_block_flag)
    {
        sb = 2;
        dct36(fsIn[0], rawout1, rawout2, win[0], tspnt);
        dct36(fsIn[1], rawout1 + 18, rawout2 + 18, win1[0], tspnt + 1);
        rawout1 += 36;
        rawout2 += 36;
        tspnt += 2;
    }
    else
    {
        sb = 0;
    }

    bt = gr_info->block_type;

    if(bt == 2)
    {
        for(; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36)
        {
            dct12(fsIn[sb], rawout1, rawout2, win[2], tspnt);
            dct12(fsIn[sb+1], rawout1 + 18, rawout2 + 18, win1[2], tspnt + 1);
        }
    }
    else
    {
        for(; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36)
        {
            dct36(fsIn[sb], rawout1, rawout2, win[bt], tspnt);
            dct36(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, win1[bt], tspnt + 1);
        }
    }

    for(; sb < SBLIMIT; sb++, tspnt++)
    {
//		int i;  // !
//		for(i=0;i<SSLIMIT;i++) {  // !
//			tspnt[i*SBLIMIT] = *rawout1++;  // !
//			*rawout2++ = 0.0;  // !
//		}  // !

        tspnt[0 * SBLIMIT] = *rawout1++;  // 0
        tspnt[1 * SBLIMIT] = *rawout1++;  // 1
        tspnt[2 * SBLIMIT] = *rawout1++;  // 2
        tspnt[3 * SBLIMIT] = *rawout1++;  // 3
        tspnt[4 * SBLIMIT] = *rawout1++;  // 4
        tspnt[5 * SBLIMIT] = *rawout1++;  // 5
        tspnt[6 * SBLIMIT] = *rawout1++;  // 6
        tspnt[7 * SBLIMIT] = *rawout1++;  // 7
        tspnt[8 * SBLIMIT] = *rawout1++;  // 8
        tspnt[9 * SBLIMIT] = *rawout1++;  // 9
        tspnt[10 * SBLIMIT] = *rawout1++;  // 10
        tspnt[11 * SBLIMIT] = *rawout1++;  // 11
        tspnt[12 * SBLIMIT] = *rawout1++;  // 12
        tspnt[13 * SBLIMIT] = *rawout1++;  // 13
        tspnt[14 * SBLIMIT] = *rawout1++;  // 14
        tspnt[15 * SBLIMIT] = *rawout1++;  // 15
        tspnt[16 * SBLIMIT] = *rawout1++;  // 16
        tspnt[17 * SBLIMIT] = *rawout1++;  // 17

        memset(rawout2, 0, SSLIMIT * sizeof(real));  // !
        rawout2 += SSLIMIT;  // !
    }
}

/*
 * main layer3 handler
 */

int __fastcall mpg123::do_layer3(unsigned char* pcm_sample, int* pcm_point)
{
    int gr, ch;
    int scalefacs[2][39]; /* max 39 for short[13][3] mode, mixed: 38, long: 22 */
    III_sideinfo sideinfo;
    int ms_stereo, i_stereo;
    int stereo1, granules;

    if(fr.stereo == 1)   /* stream is mono */
    {
        stereo1 = 1;
    }
    else
    {
        stereo1 = 2;
    }

    if(fr.mode == MPG_MD_JOINT_STEREO)
    {
        ms_stereo = (fr.mode_ext & 0x2) >> 1;
        i_stereo  = fr.mode_ext & 0x1;
    }
    else
    {
        ms_stereo = i_stereo = 0;
    }

    granules = fr.lsf ? 1 : 2;

    if(!III_get_side_info(&sideinfo, ms_stereo)) return MP3_ERR;

    if(ReservoirBuff->SetPointer(sideinfo.main_data_begin) == false)
    {
        return MP3_WAR; // -1; // changed by Otachan
    }

    int p1 = *pcm_point;

    for(gr = 0; gr < granules; gr++)
    {
        real hybridIn[2][SBLIMIT][SSLIMIT];
        struct gr_info_s *gr_info;
        long part2bits;

        gr_info = &(sideinfo.ch[0].gr[gr]);

        if(fr.lsf)
        {
            part2bits = III_get_scale_factors_2(scalefacs[0], gr_info, 0);
        }
        else
        {
#ifdef MPEG1
            part2bits = III_get_scale_factors_1(scalefacs[0], gr_info);
#else
//			fprintf(stderr,"Not supported\n");
#endif
        }

        if(III_dequantize_sample(hybridIn[0], scalefacs[0], gr_info, part2bits))
        {
            return MP3_WAR; // clip; // changed by Otachan
        }

        if(fr.stereo == 2)
        {
            gr_info = &(sideinfo.ch[1].gr[gr]);

            if(fr.lsf)
            {
                part2bits = III_get_scale_factors_2(scalefacs[1], gr_info, i_stereo);
            }
            else
            {
#ifdef MPEG1
                part2bits = III_get_scale_factors_1(scalefacs[1], gr_info);
#else
//				fprintf(stderr,"Not supported\n");
#endif
            }

            if(III_dequantize_sample(hybridIn[1], scalefacs[1], gr_info, part2bits))
            {
                return MP3_WAR; // clip; // changed by Otachan
            }

            if(ms_stereo)
            {
                unsigned int i;
                unsigned int maxb = sideinfo.ch[0].gr[gr].maxb;

                if(sideinfo.ch[1].gr[gr].maxb > maxb) maxb = sideinfo.ch[1].gr[gr].maxb;

                for(i = 0; i < SSLIMIT * maxb; i++)
                {
                    real tmp0 = ((real*)hybridIn[0])[i];
                    real tmp1 = ((real*)hybridIn[1])[i];

                    ((real*)hybridIn[0])[i] = tmp0 + tmp1;
                    ((real*)hybridIn[1])[i] = tmp0 - tmp1;
                }
            }

            if(i_stereo) III_i_stereo(hybridIn, scalefacs[1], gr_info, ms_stereo);

            if(ms_stereo || i_stereo)
            {
                if(gr_info->maxb > sideinfo.ch[0].gr[gr].maxb)
                {
                    sideinfo.ch[0].gr[gr].maxb = gr_info->maxb;
                }
                else
                {
                    gr_info->maxb = sideinfo.ch[0].gr[gr].maxb;
                }
            }
        }

        real hybridOut[2][SSLIMIT][SBLIMIT];

        for(ch = 0; ch < stereo1; ch++)
        {
            gr_info = &(sideinfo.ch[ch].gr[gr]);

            III_antialias(hybridIn[ch], gr_info);
            III_hybrid(hybridIn[ch], hybridOut[ch], ch, gr_info);
        }

        if(stereo1 == 1)
        {
#if !defined(MPG123_USE_ONLY_FLOAT64) || defined(MPG123_USE_GAIN)
            (this->*synth_1to1_mono_func)(hybridOut[0][0], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][1], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][2], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][3], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][4], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][5], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][6], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][7], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][8], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][9], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][10], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][11], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][12], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][13], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][14], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][15], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][16], pcm_sample, pcm_point);
            (this->*synth_1to1_mono_func)(hybridOut[0][17], pcm_sample, pcm_point);
#else
            synth_1to1_mono_float64(hybridOut[0][0], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][1], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][2], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][3], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][4], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][5], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][6], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][7], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][8], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][9], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][10], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][11], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][12], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][13], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][14], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][15], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][16], pcm_sample, pcm_point);
            synth_1to1_mono_float64(hybridOut[0][17], pcm_sample, pcm_point);
#endif
        }
        else
        {
#if !defined(MPG123_USE_ONLY_FLOAT64) || defined(MPG123_USE_GAIN)
            (this->*synth_1to1_func)(hybridOut[0][0], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][0], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][1], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][1], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][2], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][2], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][3], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][3], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][4], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][4], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][5], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][5], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][6], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][6], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][7], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][7], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][8], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][8], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][9], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][9], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][10], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][10], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][11], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][11], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][12], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][12], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][13], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][13], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][14], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][14], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][15], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][15], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][16], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][16], 1, pcm_sample, pcm_point);
            (this->*synth_1to1_func)(hybridOut[0][17], 0, pcm_sample, &p1);
            (this->*synth_1to1_func)(hybridOut[1][17], 1, pcm_sample, pcm_point);
#else
            synth_1to1_float64(hybridOut[0][0], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][0], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][1], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][1], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][2], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][2], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][3], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][3], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][4], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][4], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][5], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][5], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][6], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][6], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][7], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][7], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][8], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][8], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][9], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][9], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][10], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][10], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][11], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][11], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][12], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][12], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][13], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][13], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][14], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][14], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][15], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][15], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][16], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][16], 1, pcm_sample, pcm_point);
            synth_1to1_float64(hybridOut[0][17], 0, pcm_sample, &p1);
            synth_1to1_float64(hybridOut[1][17], 1, pcm_sample, pcm_point);
#endif
        }
    }

    return MP3_OK;
}

