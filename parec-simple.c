/***
  This file is part of PulseAudio.
  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.
  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <errno.h>
#include <fftw3.h>
#include <pulse/error.h>
#include <pulse/simple.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024
/* A simple routine calling UNIX write() in a loop */
static ssize_t loop_write(int fd, const void* data, size_t size)
{
    ssize_t ret = 0;
    while (size > 0) {
        ssize_t r;
        if ((r = write(fd, data, size)) < 0)
            return r;
        if (r == 0)
            break;
        ret += r;
        data = (const uint8_t*)data + r;
        size -= (size_t)r;
    }
    return ret;
}
int main(int argc, char* argv[])
{
    /* The sample type to use */
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE, //Signed 16 Bit PCM, native endian.
        .rate = 44100,
        .channels = 1
    };
    pa_simple* s = NULL;
    int ret = 1;
    int error;
    /* Create the recording stream */
    if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

    int16_t buf[BUFSIZE];
    double fft_in[BUFSIZE];

    fftw_complex *fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (2*(BUFSIZE / 2 + 1));
    fftw_plan fft_plan = fftw_plan_dft_r2c_1d(BUFSIZE, fft_in, fft_out, FFTW_FORWARD);


    /*     (')
     *    / /
     *   / /
     *  ( ) )
     */

    for (;;) {
        /* Record some data ... */
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
            fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }

        int i, j;
        /* for (i = 0; i < BUFSIZE; i += 16) { */
        /*     double normalized = (double)buf[i] / (double)((1 << 15) - 1); */
        /*     for (j = 0; j < (normalized + 1) * 80; j++) */
        /*         printf(" "); */
        /*     printf("x\n"); */
        /* } */
        for (i = 0; i < BUFSIZE; i += 1) {
            fft_in[i] = (double)buf[i] / (double)((1 << 15) - 1);
        }

        fftw_execute(fft_plan);
        /* And write it to STDOUT */
        /* if (loop_write(STDOUT_FILENO, buf, sizeof(buf)) != sizeof(buf)) { */
        /*     fprintf(stderr, __FILE__ ": write() failed: %s\n", strerror(errno)); */
        /*     goto finish; */
        /* } */
    }
    ret = 0;
finish:
    if (s)
        pa_simple_free(s);
    return ret;
}
