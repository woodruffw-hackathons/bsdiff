/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2014 William Woodruff (william @ tuffbizz.com)
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <bzlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static off_t offtin(uint8_t *buf)
{
	off_t y;

	y = buf[7] & 0x7F;
	y = y * 256;
	y += buf[6];
	y = y * 256;
	y += buf[5];
	y = y * 256;
	y += buf[4];
	y = y * 256;
	y += buf[3];
	y = y * 256;
	y += buf[2];
	y = y * 256;
	y += buf[1];
	y = y * 256;
	y += buf[0];

	if (buf[7] & 0x80)
		y = -y;

	return y;
}

int main(int argc, char *argv[])
{
	FILE *f, *cpf, *dpf, *epf;
	BZFILE *cpfbz2, *dpfbz2, *epfbz2;
	int cbz2err, dbz2err, ebz2err;
	int fd;
	ssize_t oldsize, newsize;
	ssize_t bzctrllen, bzdatalen;
	uint8_t header[32], buf[8];
	uint8_t *old, *new;
	off_t oldpos, newpos;
	off_t ctrl[3];
	off_t lenread;
	off_t i;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s oldfile newfile patchfile\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Open patch file */
	if ((f = fopen(argv[3], "r")) == NULL)
	{
		fprintf(stderr, "fopen(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	/*
	File format:
		0	8	"BSDIFFXX"
		8	8	X
		16	8	Y
		24	8	sizeof(newfile)
		32	X	bzip2(control block)
		32+X	Y	bzip2(diff block)
		32+X+Y	???	bzip2(extra block)
	with control block a set of triples (x,y,z) meaning "add x bytes
	from oldfile to x bytes from the diff block; copy y bytes from the
	extra block; seek forwards in oldfile by z bytes".
	*/

	/* Read header */
	if (fread(header, 1, 32, f) < 32)
	{
		if (feof(f))
		{
			fprintf(stderr, "Corrupt patch\n");
			exit(EXIT_FAILURE);
		}
		
		fprintf(stderr, "fread(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFFXX", 8) != 0)
	{
		fprintf(stderr, "Corrupt patch\n");
		exit(EXIT_FAILURE);
	}

	/* Read lengths from header */
	bzctrllen = offtin(header + 8);
	bzdatalen = offtin(header + 16);
	newsize = offtin(header + 24);
	if ((bzctrllen < 0) || (bzdatalen < 0) || (newsize < 0))
	{
		fprintf(stderr, "Corrupt patch\n");
		exit(EXIT_FAILURE);
	}

	/* Close patch file and re-open it via libbzip2 at the right places */
	if (fclose(f))
	{
		fprintf(stderr, "fclose(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	if ((cpf = fopen(argv[3], "r")) == NULL)
	{
		fprintf(stderr, "fopen(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	if (fseeko(cpf, 32, SEEK_SET))
	{
		fprintf(stderr, "fseeko(%s, %d)\n", argv[3], 32);
		exit(EXIT_FAILURE);
	}

	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL)
	{
		fprintf(stderr, "BZ2_bzReadOpen, bz2err = %d\n", cbz2err);
		exit(EXIT_FAILURE);
	}

	if ((dpf = fopen(argv[3], "r")) == NULL)
	{
		fprintf(stderr, "fopen(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	if (fseeko(dpf, 32 + bzctrllen, SEEK_SET))
	{
		fprintf(stderr, "fseeko(%s, %lld)\n", argv[3], (long long) (32 + bzctrllen));
		exit(EXIT_FAILURE);
	}

	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL)
	{
		fprintf(stderr, "BZ2_bzReadOpen, bz2err = %d\n", dbz2err);
		exit(EXIT_FAILURE);
	}

	if ((epf = fopen(argv[3], "r")) == NULL)
	{
		fprintf(stderr, "fopen(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	if (fseeko(epf, 32 + bzctrllen + bzdatalen, SEEK_SET))
	{
		fprintf(stderr, "fseeko(%s, %lld)\n", argv[3], (long long) (32 + bzctrllen + bzdatalen));
		exit(EXIT_FAILURE);
	}
	
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL)
	{
		fprintf(stderr, "BZ2_bzReadOpen, bz2err = %d\n", ebz2err);
		exit(EXIT_FAILURE);
	}

	if (((fd = open(argv[1], O_RDONLY, 0)) < 0) ||
		((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
		((old = malloc(oldsize+1)) == NULL) ||
		(lseek(fd, 0, SEEK_SET) != 0) ||
		(read(fd, old, oldsize) != oldsize) ||
		(close(fd) == -1))
	{
		fprintf(stderr, "%s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if ((new = malloc(newsize + 1)) == NULL)
		exit(EXIT_FAILURE);

	oldpos = 0;
	newpos = 0;
	while (newpos < newsize)
	{
		/* Read control data */
		for (i = 0;i <= 2; i++)
		{
			lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);

			if ((lenread < 8) || ((cbz2err != BZ_OK) &&
			    (cbz2err != BZ_STREAM_END)))
			{
				fprintf(stderr, "Corrupt patch\n");
				exit(EXIT_FAILURE);
			}

			ctrl[i]=offtin(buf);
		}

		/* Sanity-check */
		if (newpos + ctrl[0] > newsize)
		{
			fprintf(stderr, "Corrupt patch\n");
			exit(EXIT_FAILURE);
		}

		/* Read diff string */
		lenread = BZ2_bzRead(&dbz2err, dpfbz2, new + newpos, ctrl[0]);
		if ((lenread < ctrl[0]) ||
		    ((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END)))
		{
			fprintf(stderr, "Corrupt patch\n");
			exit(EXIT_FAILURE);
		}

		/* Add old data to diff string */
		for (i=0; i < ctrl[0]; i++)
			if ((oldpos + i >= 0) && (oldpos + i < oldsize))
				new[newpos + i] += old[oldpos + i];

		/* Adjust pointers */
		newpos += ctrl[0];
		oldpos += ctrl[0];

		/* Sanity-check */
		if (newpos + ctrl[1] > newsize)
		{
			fprintf(stderr, "Corrupt patch\n");
			exit(EXIT_FAILURE);
		}

		/* Read extra string */
		lenread = BZ2_bzRead(&ebz2err, epfbz2, new + newpos, ctrl[1]);
		if ((lenread < ctrl[1]) ||
		    ((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END)))
		{
			fprintf(stderr, "Corrupt patch\n");
			exit(EXIT_FAILURE);
		}

		/* Adjust pointers */
		newpos += ctrl[1];
		oldpos += ctrl[2];
	}

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&cbz2err, cpfbz2);
	BZ2_bzReadClose(&dbz2err, dpfbz2);
	BZ2_bzReadClose(&ebz2err, epfbz2);
	if (fclose(cpf) || fclose(dpf) || fclose(epf))
	{
		fprintf(stderr, "fclose(%s)\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	/* Write the new file */
	if (((fd = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, 0666)) < 0) ||
		(write(fd, new, newsize) != newsize) || (close(fd) == -1))
	{
		fprintf(stderr, "%s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	free(new);
	free(old);

	return EXIT_SUCCESS;
}
