/* du - report on disk usage		Author: Alistair G. Crooks */

/*
 *	du.c		1.1	27/5/87		agc	Joypace Ltd.
 *
 *	Copyright 1987, Joypace Ltd., London UK. All rights reserved.
 *	This code may be freely distributed, provided that this notice
 *	remains attached.
 *
 *	du - a public domain interpretation of du(1).
 *
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <minix/blocksize.h>

char *prog;				/* program name */
char *optstr = "as";			/* -a and -s arguments */
int silent = 0;				/* silent mode */
int all = 0;				/* all directory entries mode */
char *startdir = ".";			/* starting from here */

#define	LINELEN 256

#define DIRNAMELEN 14
#define	LSTAT stat
typedef struct _dirstr {
  int	inum;
  char	d_name[DIRNAMELEN];
} DIR;
DIR dir;
#define ino_t unsigned short

typedef struct _alstr {
  int al_dev;
  ino_t	al_inum;
} ALREADY;

#define	MAXALREADY	50
ALREADY already[MAXALREADY];
int alc = 0;

/*
 *	myindex - stop the scanf bug
 */
char *myindex(s, c)
register char	*s;
register char	c;
{
  for (; *s; s++)
	if (*s == c)
		return(s);
  return(NULL);
}


/*
 *	getopt - parse the arguments given.
 *	retrieved from net.sources
 */
int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

#define BADCH	(int)'?'
#define EMSG	""
#define TELL(s)	fputs(*nargv, stderr); fputs(s, stderr);\
	fputc(optopt, stderr); fputc('\n', stderr);\
	return(BADCH);

int getopt(nargc, nargv, ostr)
int	nargc;
char	**nargv;
char	*ostr;
{
  register char	*oli;
  static char	*place = EMSG;

  if (!*place) {
	if (optind >= nargc || *(place = nargv[optind]) != '-' || !*++place)
		return(EOF);
	if (*place == '-') {
		++optind;
		return(EOF);
	}
  }
  if ((optopt = (int)*place++) == (int)':' || !(oli = myindex(ostr, optopt))) {
	if (!*place)
		++optind;
	TELL(": illegal option -- ");
  }
  if (*++oli != ':') {
	optarg = NULL;
	if (!*place)
		++optind;
  } else {
	if (*place)
		optarg = place;
	else if (nargc <= ++optind) {
		place = EMSG;
		TELL(": option requires an argument -- ");
	} else
		optarg = nargv[optind];
	place = EMSG;
	++optind;
  }
  return(optopt);
}

/*
 *	makedname - make the direcory entry from the directory name,
 *	and the file name, placing it in out. If this would overflow,
 *	return 0, otherwise 1.
 */
int makedname(d, f, out, outlen)
char	*d;
char	*f;
char	*out;
int	outlen;
{
  char	*cp;

  if (strlen(d) + strlen(f) + 2 > outlen)
	return(0);
  for (cp = out ; *d ; *cp++ = *d++)
	;
  if (*(cp-1) != '/')
	*cp++ = '/';
  while (*f)
	*cp++ = *f++;
  *cp = '\0';
  return(1);
}

/*
 *	done - have we encountered (dev, inum) before? Returns 1 for yes,
 *	0 for no, and remembers (dev, inum).
 */
int done(dev, inum)
int	dev;
ino_t	inum;
{
  register ALREADY	*ap;
  register int		i;
  int			ret = 0;

  for (i = 0, ap = already ; i < alc ; ap++, i++)
	if (ap->al_dev == dev && ap->al_inum == inum) {
		ret = 1;
		break;
	}
  if (alc < MAXALREADY) {
	already[alc].al_dev = dev;
	already[alc++].al_inum = inum;
  }
  return(ret);
}

/*
 *	dodir - process the directory d. Return the long size (in blocks)
 *	of d and its descendants.
 */
long dodir(d)
char	*d;
{
  struct stat	s;
  long		total = 0L;
  char		dent[LINELEN];
  int		fd;
  
  if ((fd = open(d, 0)) < 0)
	return(0L);
  while (read(fd, &dir, sizeof(dir)) > 0) {
	if (strcmp(dir.d_name, ".") == 0 ||
	    strcmp(dir.d_name, "..") == 0)
		continue;
	if (dir.inum == 0)
		continue;
	if (!makedname(d, dir.d_name, dent, sizeof(dent)))
		continue;
	if (LSTAT(dent, &s) < 0)
		continue;
	if (s.st_nlink > 1 && done(s.st_dev, s.st_ino))
		continue;
	if ((s.st_mode & S_IFMT) == S_IFDIR)
		total += dodir(dent);
	switch(s.st_mode & S_IFMT) {
	case S_IFREG:
	case S_IFDIR:
		total += (s.st_size + BLOCK_SIZE) / BLOCK_SIZE;
		break;
	}
	if (all && (s.st_mode & S_IFMT) != S_IFDIR)
		printf("%D\t%s\n", (s.st_size + BLOCK_SIZE) / BLOCK_SIZE, dent);
  }
  close(fd);
  if (!silent)
	printf("%D\t%s\n", total, d);
  return(total);
}

/*
 *	OK, here goes...
 */
main(argc, argv)
int	argc;
char	**argv;
{
  long	tot;
  int	c;

  prog = argv[0];
  while ((c = getopt(argc, argv, optstr)) != EOF)
	switch(c) {
	case 'a' :
		all = 1;
		break;
	case 's' :
		silent = 1;
		break;
	default :
		fprintf(stderr, "Usage: %s [-a] [-s] [startdir]\n", prog);
		exit(1);
	}
  if (optind < argc)
	startdir = argv[optind];
  tot = dodir(startdir);
  if (silent)
	printf("%D\t%s\n", tot, startdir);
  exit(0);
}
