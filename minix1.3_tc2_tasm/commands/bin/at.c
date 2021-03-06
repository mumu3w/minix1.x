/* at - run a command at a specified time	Author: Jan Looyen */


#define		DIR		"/usr/spool/at/
#define		STARTDAY	0		/*  see ctime(3)	*/
#define		LEAPDAY		STARTDAY+59
#define		MAXDAYNR	STARTDAY+365
#define		NODAY		-2

#include	<stdio.h>
#include	<sys/types.h>
#include	<time.h>

main(argc, argv, envp)
int  argc;
char **argv, **envp;
{
    int		i, count, ltim, year, getltim(), getlday(), lday = NODAY;
    char	c, buf[10], job[30], *dp, *sp;
    struct tm	*p, *localtime();
    long	clock;
    FILE	*fp, *pin, *popen();

/*-------------------------------------------------------------------------*
 *	check arguments	& pipe to "pwd"				           *
 *-------------------------------------------------------------------------*/
    if (argc < 2 || argc > 5) {
	fprintf(stderr, "Usage: %s time [month day] [file]\n", argv[0]);
	exit(1);
    }
    if ((ltim = getltim(argv[1])) == -1) {
	fprintf(stderr, "%s: wrong time specification\n", argv[0]);
	exit(1);
    }
    if ((argc==4 || argc==5) && (lday = getlday(argv[2], argv[3]))==-1) {
	fprintf(stderr, "%s: wrong date specification\n", argv[0]);
	exit(1);
    }
    if ((argc==3 || argc==5) && open(argv[argc-1], 0) == -1) {
	fprintf(stderr, "%s: cannot find: %s\n", argv[0], argv[argc-1]);
	exit(1);
    }
    if ((pin = popen("pwd", "r")) == NULL) {
	fprintf(stderr, "%s: cannot open pipe to cmd 'pwd'\n", argv[0]);
	exit(1);
    }
/*-------------------------------------------------------------------------*
 *	determine execution time and create 'at' job file		   *
 *-------------------------------------------------------------------------*/
    time(&clock);
    p = localtime(&clock);
    year = p->tm_year;
    if (lday==NODAY) {				   /* no [month day] given */
	lday = p->tm_yday;
	if (ltim <= (p->tm_hour*100 + p->tm_min)) {
	    lday++;
	    if (lday==MAXDAYNR && (year%4) || lday==MAXDAYNR+1) {
		lday = STARTDAY;
		year++;
	    }
	}
    }
    else
	switch (year%4) {
	    case 0: if (lday < p->tm_yday || lday == p->tm_yday &&
			ltim <= (p->tm_hour*100 + p->tm_min)      ) {
			year++;
			if (lday > LEAPDAY) lday-- ;
		    }
		    break;
	    case 1:
	    case 2: if (lday > LEAPDAY) lday-- ;
		    if (lday < p->tm_yday || lday == p->tm_yday &&
			ltim <= (p->tm_hour*100 + p->tm_min)      )
			year++;
		    break;
	    case 3: if (lday < ((lday > LEAPDAY) ? p->tm_yday+1 : p->tm_yday) ||
			lday== ((lday > LEAPDAY) ? p->tm_yday+1 : p->tm_yday) &&
			ltim <= (p->tm_hour*100 + p->tm_min)		    )
			year++;
		    else if (lday > LEAPDAY) lday--;
		    break;
	}
    sprintf(job, DIR%02d.%03d.%04d.%02d", year%100, lday, ltim, getpid()%100);
    if ((fp = fopen(job, "w")) == NULL) {
	fprintf(stderr, "%s: cannot create %s\n", argv[0], job);
	exit(1);
    }
/*-------------------------------------------------------------------------*
 *	write environment and command(s) to 'at'job file		   *
 *-------------------------------------------------------------------------*/
    while (envp[i] != NULL) {
	count = 1;
	dp = buf;
	sp = envp[i];
	while ((*dp++ = *sp++) != '=')
	    count++;
	*--dp = '\0';
	fprintf(fp, "export %s; %s='%s'\n", buf, buf, &envp[i++][count]);
    }
    fprintf(fp, "cd ");
    while ((c = getc(pin)) != EOF)
	putc(c, fp);
    fprintf(fp, "umask %o\n", umask());
    if (argc==3 || argc==5)
	fprintf(fp, "%s\n", argv[argc-1]);
    else 					     /* read from stdinput */
	while ((c = getchar()) != EOF)
	    putc(c, fp); 

    printf("%s: %s created\n", argv[0], job);
    exit(0);
}

/*-------------------------------------------------------------------------*
 *	getltim()		return((time OK) ? daytime : -1)	   *
 *-------------------------------------------------------------------------*/
getltim(t)
char *t;
{
    if (t[4] == '\0' && t[3] >= '0' && t[3] <= '9' &&
        t[2] >= '0'  && t[2] <= '5' && t[1] >= '0' && t[1] <= '9' &&
        (t[0] == '0' || t[0] == '1' || t[1] <= '3' && t[0] == '2')   )
	return(atoi(t));
    else
	return(-1);
}

/*-------------------------------------------------------------------------*
 *	getlday()		return ((date OK) ? yearday : -1)	   *
 *-------------------------------------------------------------------------*/
getlday(m, d)
char *m, *d;
{
    int i, month, day, im;
    static int cumday[] = { 0, 0, 31, 60, 91, 121, 152,
 			  182, 213, 244, 274, 305, 335 };
    static struct date {
	   char *mon;
	   int dcnt;
    }   *pc,
        kal[] = { "Jan", 31, "Feb", 29, "Mar", 31, "Apr", 30,
		  "May", 31, "Jun", 30, "Jul", 31, "Aug", 31,
		  "Sep", 30, "Oct", 31, "Nov", 30, "Dec", 31
		};

    pc = kal;
    im = (digitstring(m)) ? atoi(m) : 0;
    m[0] &= 0337;
    for (i = 1; i < 13 && strcmp(m, pc->mon) && im != i; i++, pc++)
	;
    if (i < 13 && (day=(digitstring(d)) ? atoi(d) : 0) && day <= pc->dcnt) {
	if (!STARTDAY) day--;
	return(day + cumday[i]);
    }    
    else
	return(-1);
}



digitstring(s)
char *s;
{
    while (*s >= '0' && *s <= '9')
	s++;
    return((*s=='\0') ? 1 : 0);
}
 
