/*
    Copyright 2008-2014 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "mywav.h"
#include "dsp.c"
//#include "resample2.c"
#ifdef WIN32
#else
    #define stricmp strcasecmp
#endif



typedef int8_t      i8;
typedef uint8_t     u8;
typedef int16_t     i16;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;



#define VER     "0.1.1"



int mywav_fri24(FILE *fd, uint32_t *num);
i16 *do_samples(FILE *fd, int wavsize, int *ret_samples, int bits);
int do_mono(i16 *smp, int samples, int ch);
void do_dcbias(i16 *smp, int samples);
void do_normalize(i16 *smp, int samples);
int do_8000(i16 *smp, int samples, int *freq);
void my_err(u8 *err);
void std_err(void);



typedef struct {
    char    *par;
    double  *val;
} dsp_par_val_t;
dsp_par_val_t dsp_par_val[] = {
#define SET_dsp_par_val(X)   { #X, &X },
    SET_dsp_par_val(DTMF_OPTIMIZED_VALUE)
    SET_dsp_par_val(DTMF_THRESHOLD)
    SET_dsp_par_val(DTMF_NORMAL_TWIST)
    SET_dsp_par_val(DTMF_RELATIVE_PEAK_ROW)
    SET_dsp_par_val(DTMF_RELATIVE_PEAK_COL)
    SET_dsp_par_val(BELL_MF_THRESHOLD)
    SET_dsp_par_val(BELL_MF_TWIST)
    SET_dsp_par_val(BELL_MF_RELATIVE_PEAK)
    { NULL, NULL }
};



int main(int argc, char *argv[]) {
    digit_detect_state_t dtmf;
    mywav_fmtchunk  fmt;
    struct  stat    xstat;
    FILE    *fd;
    int     i,
            j,
            wavsize,
            samples,
            writeback,
            raw         = 0,
            optimize    = 1;
    i16     *smp        = NULL;
    u8      *fname,
            *outfile    = NULL;

    setbuf(stdin,  NULL);
    setbuf(stdout, NULL);


    if(argc < 2) {
        printf("\n"
            "Usage: %s [options] <file.WAV>\n"
            "\n", argv[0]);
        printf(
            "Options:\n"
            "-r F C B  consider the file as raw headerless PCM data, you must specify the\n"
            "          Frequency, Channels and Bits like -r 44100 2 16\n"
            "-o        disable the automatic optimizations: DC bias adjust and normalize.\n"
            "          use this option only if your file is already clean and normalized\n"
            "-w FILE   debug option for dumping the handled samples from the memory to FILE\n"
            "-z P V    set specific parameters of dsp.c:\n"
            "\n");
        for(i = 0; dsp_par_val[i].par; i++) {
            printf("            %-25s %.2f\n", dsp_par_val[i].par, *dsp_par_val[i].val);
        }
        printf("\n");
        exit(1);
    }

    argc--;
    for(i = 1; i < argc; i++) {
        if(((argv[i][0] != '-') && (argv[i][0] != '/')) || (strlen(argv[i]) != 2)) {
            printf("\nError: wrong argument (%s)\n", argv[i]);
            exit(1);
        }
        switch(argv[i][1]) {
            case 'r':
                memset(&fmt, 0, sizeof(fmt));
                if(!argv[++i]) exit(1);
                fmt.dwSamplesPerSec = atoi(argv[i]);
                if(!argv[++i]) exit(1);
                fmt.wChannels       = atoi(argv[i]);
                if(!argv[++i]) exit(1);
                fmt.wBitsPerSample  = atoi(argv[i]);
                fmt.wFormatTag      = 1;
                raw = 1;
                break;
                
            case 'o':
                optimize = 0;
                break;
                
            case 'w':
                if(!argv[++i]) exit(1);
                outfile = argv[i];
                break;

            case 'z':
                if(!argv[++i]) exit(1);
                for(j = 0; dsp_par_val[j].par; j++) {
                    if(!stricmp(argv[i], dsp_par_val[j].par)) {
                        sscanf(argv[++i], "%lf", dsp_par_val[j].val);
                        break;
                    }
                }
                break;
            
            default:
                printf("\nError: wrong option (%s)\n", argv[i]);
                exit(1);
                break;
        }
    }

    fname = argv[argc];

    if(!strcmp(fname, "-")) {
        fd = stdin;
    } else {
        fd = fopen(fname, "rb");
        if(!fd) std_err();
    }

    if(raw) {
        fstat(fileno(fd), &xstat);
        wavsize = xstat.st_size;
    } else {
        wavsize = mywav_data(fd, &fmt);
    }

    if(wavsize <= 0) my_err("corrupted WAVE file");
    if(fmt.wFormatTag != 1) my_err("only the classical PCM WAVE files are supported");

    smp = do_samples(fd, wavsize, &samples, fmt.wBitsPerSample);
    if(fd != stdin) fclose(fd);

    samples = do_mono(smp, samples, fmt.wChannels);
    if(optimize) {
        do_dcbias(smp, samples);
        do_normalize(smp, samples);
    }
    samples = do_8000(smp, samples, &fmt.dwSamplesPerSec);

    fmt.wFormatTag       = 0x0001;
    fmt.wChannels        = 1;
    fmt.wBitsPerSample   = 16;
    fmt.wBlockAlign      = (fmt.wBitsPerSample >> 3) * fmt.wChannels;
    fmt.dwAvgBytesPerSec = fmt.dwSamplesPerSec * fmt.wBlockAlign;
    wavsize              = samples * sizeof(* smp);

    if(outfile) {
        fd = fopen(outfile, "wb");
        if(!fd) std_err();
        mywav_writehead(fd, &fmt, wavsize, NULL, 0);
        fwrite(smp, 1, wavsize, fd);
        fclose(fd);
    }

    SAMPLE_RATE = fmt.dwSamplesPerSec;

    memset(&dtmf, 0, sizeof(dtmf)); // useless
    ast_digit_detect_init(&dtmf, DSP_DIGITMODE_MF);
    mf_detect(&dtmf, smp, samples, DSP_DIGITMODE_NOQUELCH, &writeback);
    printf("%s", dtmf.digits[0] ? dtmf.digits : "");

    if(smp) free(smp);
    return(0);
}



int mywav_fri24(FILE *fd, uint32_t *num) {
    uint32_t    ret;
    uint8_t     tmp;

    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret = tmp;
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 8);
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 16);
    *num = ret;
    return(0);
}



i16 *do_samples(FILE *fd, int wavsize, int *ret_samples, int bits) {
    i32     tmp32;
    int     i   = 0,
            samples;
    i16     *smp;
    i8      tmp8;

    samples = wavsize / (bits >> 3);
    smp = malloc(sizeof(* smp) * samples);
    if(!smp) std_err();

    if(bits == 8) {
        for(i = 0; i < samples; i++) {
            if(mywav_fri08(fd, &tmp8) < 0) break;
            smp[i] = (tmp8 << 8) - 32768;
        }

    } else if(bits == 16) {
        for(i = 0; i < samples; i++) {
            if(mywav_fri16(fd, &smp[i]) < 0) break;
        }

    } else if(bits == 24) {
        for(i = 0; i < samples; i++) {
            if(mywav_fri24(fd, &tmp32) < 0) break;
            smp[i] = tmp32 >> 8;
        }

    } else if(bits == 32) {
        for(i = 0; i < samples; i++) {
            if(mywav_fri32(fd, &tmp32) < 0) break;
            smp[i] = tmp32 >> 16;
        }

    } else {
        my_err("number of bits used in the WAVE file not supported");
    }
    *ret_samples = i;
    return(smp);
}



int do_mono(i16 *smp, int samples, int ch) {
    i32     tmp;    // max 65535 channels
    int     i,
            j;

    if(!ch) my_err("the WAVE file doesn't have channels");
    if(ch == 1) return(samples);

    for(i = 0; samples > 0; i++) {
        tmp = 0;
        for(j = 0; j < ch; j++) {
            tmp += smp[(i * ch) + j];
        }
        smp[i] = tmp / ch;
        samples -= ch;
    }
    return(i);
}



void do_dcbias(i16 *smp, int samples) {
    int     i;
    i16     bias,
            maxneg,
            maxpos;

    maxneg = 32767;
    maxpos = -32768;
    for(i = 0; i < samples; i++) {
        if(smp[i] < maxneg) {
            maxneg = smp[i];
        } else if(smp[i] > maxpos) {
            maxpos = smp[i];
        }
    }

    bias = (maxneg + maxpos) / 2;

    for(i = 0; i < samples; i++) {
        smp[i] -= bias;
    }
}



void do_normalize(i16 *smp, int samples) {
    int     i;
    double  t;
    i16     bias,
            maxneg,
            maxpos;

    maxneg = 0;
    maxpos = 0;
    for(i = 0; i < samples; i++) {
        if(smp[i] < maxneg) {
            maxneg = smp[i];
        } else if(smp[i] > maxpos) {
            maxpos = smp[i];
        }
    }


    if(maxneg < 0) maxneg = (-maxneg) - 1;
    if(maxneg > maxpos) {
        bias = maxneg;
    } else {
        bias = maxpos;
    }
    if(bias >= 32767) return;


    for(i = 0; i < samples; i++) {
        t = smp[i];
        t = (t * (double)32767) / (double)bias;
        if(t > (double)32767)  t = 32767;
        if(t < (double)-32768) t = -32768;
        smp[i] = t;
    }
}



void *av_resample_init(int, int, int, int, int, double);
int av_resample(void *, short *, short *, int *, int, int, int);
void av_resample_close(void *);

int do_8000(i16 *smp, int samples, int *freq) {
    void    *res    = NULL;
    int     consumed;

    if(*freq <= 8000) return(samples);

    res = av_resample_init(8000, *freq, 16, 10, 0, 0.8);
    samples = av_resample(res, smp, smp, &consumed, samples, samples, 1);
    av_resample_close(res);

    *freq = 8000;
    return(samples);
}



void my_err(u8 *err) {
    fprintf(stderr, "\nError: %s\n", err);
    exit(1);
}



void std_err(void) {
    perror("\nError");
    exit(1);
}


