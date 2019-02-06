/*	dos2out  -  convert ms-dos exe-format to a.out format
 *		    december '85
 *
 *
 *  Modified By Deborah Mullen for Turbo C exe files.
 *  Feb. 15, 1989
 *
 *   Usage:  dos2out [-ipd] infile[.exe]
 *
 *      options: 
 *          -p  Print info about sizes of infile and outfile
 *          -d  Delete input file when done
 *          -i  Generate output file in separate I & D format
 *                 Input file assumed linked with crtsos.obj
 *
 *       Defaults:
 *          assumes input file linked with crtsot.obj
 *
 *       Output file name will have extention of:
 *             .out  for combined I&D (default)
 *             .sep  for separate I&D
 *
 *       If the output option chosen does not match the format of
 *       the input file, the result will not run.
 *       In all cases, the C source code must be compiled with the
 *       tiny model.
 *
 *	 Be forwarned that the .map file generated by Turbo C is not
 *       always correct in terms of the data size. It appears to include
 *       padding to the bss segment.
 *
 *	The way the text size is calculated is DIFFERENT for combined
 *	and separate I&D space.  In both cases there must be only 1
 *	relocation item in the .exe header.  This item is forced there
 *	by having the statement " mov ax, DGROUP" somewhere in the code.
 *      For combined I&D this statement MUST be at the very end of the
 *      code segemnt.  For Separate I&D it can be anywhere in the code 
 *	segment. For Combined I&D the text size is determined only
 *      from the relocation item.  For Separate I&D, the text size is
 *	determined by reading the word in the load image pointed to
 *	by the relocation item.
 *

Description of file-structures:

         a.out                                   .exe
-------------------------               -------------------------
|        header         |               |        header         |
|                       |               |          +            |
+-----------------------+               |      relocation       |
|        text           |               |         info          |
|          +            |               +-----------------------+
|        data           |               |                       |
+-----------------------+               |                       |
|   relocation info     |               |        text           |
+-----------------------+               |          +            |
|    symbol table       |               |	 data           |
+-----------------------+               |                       |
|    string table       |               |                       |
+-----------------------+               +-----------------------+



For more information see MS-DOS programmers reference manual

*/



/* define the a.out header format (short form) */


/* define the formatted part of the header of an MS-DOS exe-file */


#define	D_FMT_SIZE	 28		/* size of formatted header part */	
#define IS_EXE	     0x5A4D		/* valid linker signature; note that
					   in a dump it shows as 4D 5A */
#define NOP 0x90


typedef struct d_reloctab {		/* DOS relocation table entry */
	unsigned int	r_offset;	/* offset to symbol in load-module */
	unsigned int	r_segment;	/* segment relative to zero;
					   real segment must now be added */
};

/* dos formatted part of the header */
typedef struct d_fmt_hdr {
	unsigned int 	ldsign;		/* linker signature, 5A4Dh */
	unsigned int	last_page_size; /* size of last page */
	unsigned int	nr_of_pages;	/* a page is 512 b */
	unsigned int	nr_of_relocs;	/* nr of relocation items */
	unsigned int	hdr_size;	/* header-size in *16b */
	unsigned int	min_free;	/* min free mem required after prog */
	unsigned int	max_free;	/* max free mem ever needed (FFFFh) */
	unsigned int	ss_seg;		/* stack-segment; must be relocated */
	unsigned int	sp_reg;		/* sp value to be loaded */
	unsigned int	chksum;		/* neg. sum of all words in file */
	unsigned int	ip_reg;		/* ip value (program entry-point) */
	unsigned int	cs_seg;		/* offset of code-seg in load-module */
	unsigned int	reloc_table;	/* start of reloc-table in hdr, 1Bh */
	unsigned int	overlay_num;	/* overlay number - not used */
};

#define MAGIC0	       0x01		/* magic number, byte 1 */
#define MAGIC1	       0x03		/* and second byte */
#define A_SEP	       0x20		/* seperate I&D flag */
#define A_NOTSEP       0x10           /* not separate I&D flag */
#define A_EXEC	       0x10		/* executable */
#define A_I8086	       0x04		/* Intel 8088/86 cpu */
#define A_HDR_LEN	 32		/* short form of header */
#define DOS	     0xFFFF		/* version for a.out is -1 */


typedef struct a_out_hdr {
	unsigned char	a_magic[2];	/* magic number */
	unsigned char	a_flags;	/* flags for sep I&D etc */
	unsigned char   a_cpu;		/* cpu-type */
	unsigned char	a_hdrlen;	/* length of header */
	unsigned char	a_unused;	/* sic */
	unsigned short	a_version;	/* version stamp */
	long		a_text;		/* size of text segment in bytes */
	long		a_data;		/* size of data-segment in bytes */
	long		a_bss;		/* size of bss (stack) segment   */
	long		a_entry;	/* program entry-point */
	long		a_totb;		/* other, eg initial stack-ptr */
	long		a_syms;		/* symbol-table size */
	/* END OF SHORT FORM */
};

/* bytes & words left to right? */
#define A_BLR(cputype)	((cputype&0x01)!=0)  /* TRUE if bytes left-to-right */
#define A_WLR(cputype)	((cputype&0x02)!=0)  /* TRUE if words left-to-right */


#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>

#define PH_SECTSIZE	512		/* size of a disk-block */
#define NOT_A_HDR_LEN	(PH_SECTSIZE-A_HDR_LEN)	/* complement */
#define FNAME		 32		/* length of filename/path +1 */


unsigned char inbuf[PH_SECTSIZE];
unsigned char inbuf2[PH_SECTSIZE];
unsigned char outbuf[PH_SECTSIZE];

struct a_out_hdr *a_ptr= (struct a_out_hdr *)outbuf;
struct d_fmt_hdr *d_ptr= (struct d_fmt_hdr *)inbuf;

/* Variables gotten from the map file */

main (argc,argv)
int argc;
char *argv[];
{
  char *dptr,			/* pointer to DOS-header */
       *sptr,			/* pointer to OUT-header */
       in_name[FNAME],		/* input filename */
       out_name[FNAME];		/* output filename */

  register int i;
  int	in_cnt, 		/* # bytes read from input */
	out_cnt,		/* # bytes written to output */
	block_cnt,		/* # nr of bloks of load-module */
	lastp_cnt,		/* # bytes in last block */
	inf, outf,		/* filedescriptors */
	print=0,		/* switch: print information */
	delete=0,		/* delete exe file after processing */
	separate=0,             /* switch: separate I & D */
	sym,			/* data-relocation symbol offset */
	sym_blk, sym_off;	/* load-module block & offset in blk */
  long	d_size,			/* size of data-segment */
	t_size,			/* size of text-segment */
	load_size;		/* size of load-module in bytes */
   int t_pad, mod_size;		/* amount to pad by */

/*================================================================
 *			process parameters
 *===============================================================*/


  if (argc<2 || argc>3) {
     printf ("usage:  dos2out [-pd] fname[.ext]\n");
     exit (2);
  }

  while (--argc)
    switch (argv[argc][0]) {
      case '-': while (*++argv[argc])
		  switch (*argv[argc] & ~32) {
		    case 'P' : print=1; break;
		    case 'D' : delete =1; break;
		    case 'I' : separate=1; break;
		  default :
		  printf ("Bad switch %c, ignored\n",*argv[argc]);
		  }
		break;

      default :	/* make filenames */
	dptr=in_name; sptr=out_name;
	while (*argv[argc] && *argv[argc]!='.') {
	   *dptr++ = *argv[argc]; 
	   *sptr++ = *argv[argc]++;
	}
	/* is there an extension to the filename? */
	if (*argv[argc]=='.') {
	   while (*argv[argc]) *dptr++ = *argv[argc]++;
	   *dptr=0;
	}
	else  {
	   *dptr=0;
	   strcat(dptr,".exe");
	}
	*sptr=0;
    } /* end switch */

     /* Put extention on output name */
     if (separate)
		strcat(out_name,".sep");
	else
		strcat(out_name,".out");

    /* open & create files */
    if ((inf=open(in_name,O_BINARY | O_RDONLY)) <0) {
       printf ("input file %s not found\n",in_name);
       exit(2);
    }


    /* get first block and check conditions for conversion */
    if ((in_cnt=read(inf,inbuf,PH_SECTSIZE))!=PH_SECTSIZE) {
	printf ("read %d bytes, should be %d - abort\n",in_cnt,PH_SECTSIZE);
	exit(2);
    }
    if (d_ptr->ldsign!=IS_EXE) {
	printf ("not a valid .exe file - stop\n");
	exit(2);
    }
    if (d_ptr->nr_of_relocs>1) {
	printf ("Too many relocations - stop\n");
	exit(2);
    }
    /* see documentation for explanation of this condition */
    if (d_ptr->nr_of_relocs<1) {
	printf ("Exactly one relocation item required - can't process\n");
	exit(2);
    }
    if (d_ptr->ip_reg) {
	printf ("Warning - program entry point not at zero\n");
    }


    /* input file is ok, open output (possibly destroy existing file) */
    if ((outf=open(out_name,O_CREAT | O_RDWR |  O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) <0) {
	printf ("cannot open output %s\n",out_name);
	exit(2);
    }


/*========================= PROCESS FILE =============================*/

    /* Base on the 1 relocation item, determine text size */
   
    /* Get relocation item from exe header which points to the
     * 1 and only relocation item. 
     * See comments above on how this is used to get the text size.
     */
  
    if (!separate) {

	/* First read the segment part of the relocation item */
	t_size = (int) (inbuf[d_ptr->reloc_table+3] << 8);
	t_size += (int) (inbuf[d_ptr->reloc_table+2]);
	t_size <<= 4;    /* Since a segment value */

	/* Now get the offset part of relocation item */
	t_size += (int) (inbuf[d_ptr->reloc_table+1] << 8);
	t_size += (int) (inbuf[d_ptr->reloc_table]);

	/* Adjust the text size. Since this item points to the second
	 * to last byte in the text, need to add 2 to get real text size.
	 */
    
	t_size += 2;

	/* For Combined I & D, the data begins on a word boundary,
	 * So must adjust text size to account for any filler bytes
	 */

	mod_size = 2;
    	t_pad = t_size % mod_size;
    	t_size += (t_pad) ? mod_size - t_pad : 0;

   }
   else {
	/* Figure text for separate I & D */
	/* Assumming the item points somewhere in the first 64K */
	sym = (int) (inbuf[d_ptr->reloc_table+1] << 8);
	sym += (int) (inbuf[d_ptr->reloc_table]);

	sym_blk = sym / PH_SECTSIZE;
	sym_off = sym % PH_SECTSIZE;


	/* Read loadimage to get to word */
	while (sym_blk >= 0) {
		read(inf, inbuf2, PH_SECTSIZE);
		sym_blk--;
	}

	t_size =  inbuf2[sym_off+1] << 8;
	t_size += inbuf2[sym_off];

	t_size <<= 4;      /* since word stored there is a segment # */
    }



    /* Now calculate the image size of the .exe file */
    load_size  = (d_ptr->nr_of_pages-2) * PH_SECTSIZE + 
	d_ptr->last_page_size;

    /* Data size is just subtract t_size fromm load_size.
     * The build program will pad this value out to the bss for kernel.
     * It is VERY important that t_size + d_size equals exactly the
     * number of bytes in the load image.
     */
    d_size = load_size - t_size;

    /* reposition file */
    close (inf);
    inf=open(in_name,O_BINARY | O_RDONLY);
    in_cnt=read(inf,inbuf2,PH_SECTSIZE);

    /* make a.out header */
    a_ptr->a_magic[0] = MAGIC0;
    a_ptr->a_magic[1] = MAGIC1;
    a_ptr->a_flags    = (separate) ? A_SEP : A_NOTSEP;
    a_ptr->a_cpu      = A_I8086;
    a_ptr->a_hdrlen   = A_HDR_LEN;
    a_ptr->a_syms     = 0;
    a_ptr->a_unused   = 0;
    a_ptr->a_version  = 0;
    a_ptr->a_data     = d_size;
    a_ptr->a_text     = t_size;
    a_ptr->a_bss      = (d_ptr->min_free << 4) - d_ptr->sp_reg;
    a_ptr->a_entry    = d_ptr->ip_reg;
    a_ptr->a_totb =  d_ptr->sp_reg + d_size + a_ptr->a_bss;

    if (!separate) 
	a_ptr->a_totb += t_size;

    if (print) printinfo(separate);

    write(outf, outbuf, A_HDR_LEN);  /* write header */

    /* void remainder of first block; it holds nothing */

    /* determine nr of blocks to copy and copy them */
    block_cnt = d_ptr->nr_of_pages-1;		/* exclude header-block */
    lastp_cnt = d_ptr->last_page_size;
    while (block_cnt--) {

	if ((in_cnt=read(inf,inbuf,PH_SECTSIZE))!=PH_SECTSIZE)
	   if (block_cnt || (!block_cnt && in_cnt != lastp_cnt)) {
		printf ("read %d bytes, should be %d - abort\n",
			in_cnt,(block_cnt ? PH_SECTSIZE : lastp_cnt));
		exit(2);
	};	
	i = in_cnt;
	dptr = outbuf;
	sptr = inbuf;
	while ( i-- ) 

           *dptr++ = *sptr++;
          
	if ((out_cnt=write(outf,outbuf,in_cnt)) != in_cnt) {
		printf ("wrote %d bytes, should be %d - abort\n",out_cnt,in_cnt);
		exit(2);
	}
    }

    close (outf);
    close (inf);
    if (delete) unlink (in_name);
    exit(0);
}



printinfo (separate)
int separate;
{
  printf ("\n\nDOS-header:\n");
  printf ("          nr of pages:  %4xh    (%4d dec)\n",
	d_ptr->nr_of_pages, d_ptr->nr_of_pages);
  printf ("   bytes in last page:  %4xh    (%4d dec)\n",
	d_ptr->last_page_size, d_ptr->last_page_size);
  printf ("    min. free mem *16:  %4xh    (%4d dec)\n",
	d_ptr->min_free, d_ptr->min_free);
  printf ("           stack-size:  %4xh    (%4d dec)\n",
	d_ptr->ss_seg, d_ptr->ss_seg);
  printf ("    stack-pointer val:  %4xh    (%4d dec)\n",
	d_ptr->sp_reg, d_ptr->sp_reg);
  printf ("  program entry-point:  %4xh    (%4d dec)\n",
	d_ptr->ip_reg, d_ptr->ip_reg);
  printf ("     code-segment val:  %4xh    (%4d dec)\n",
	d_ptr->cs_seg, d_ptr->cs_seg);

  printf ("\n\nOUT-header:");
  if (separate)
     printf("   SEPARATE I & D\n");
  else
     printf("\n");
  printf ("            text-size: %6lxh   (%6ld dec)\n",
	a_ptr->a_text, a_ptr->a_text);
  printf ("            data-size: %6lxh   (%6ld dec)\n",
	a_ptr->a_data, a_ptr->a_data);
  printf ("             bss-size: %6lxh   (%6ld dec)\n",
	a_ptr->a_bss, a_ptr->a_bss);
  printf ("          entry-point: %6lxh   (%6ld dec)\n",
	a_ptr->a_entry, a_ptr->a_entry);
  printf ("           totalbytes: %6lxh   (%6ld dec)\n\n",
	a_ptr->a_totb, a_ptr->a_totb);
  printf("            text+data = %6lxh\n",a_ptr->a_text + a_ptr->a_data);
}

      
