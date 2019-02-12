/*
 * SHOWEXE displays information about the EXE file named on the
 * command line.  It can handle both the DOS and the OS/2 formats.  
 * If output is not redirected, it pauses after each section.
 * Written by David A. Schmitt.
 */ 
#include <stdio.h>
extern far pascal VioScrollUp();
extern far pascal VioWrtCharStr();
extern far pascal VioWrtNAttr();
extern char *malloc();

extern void pause();
extern void cls();
/**
*
* Old executable header, used by DOS
*
*/
struct EXE_DOS
   {
   unsigned short e_magic;    /* magic number 0x5A4D     */
   unsigned short e_cblp;     /* bytes in last page      */
   unsigned short e_cp;    /* 512-byte pages in file  */
   unsigned short e_crlc;     /* number of relocations   */
   unsigned short e_cparhdr;  /* paragraphs in header    */
   unsigned short e_minalloc; /* minimum extra paragraphs   */
   unsigned short e_maxalloc; /* maximum extra paragraphs   */
   unsigned short e_ss;    /* initial SS value     */
   unsigned short e_sp;    /* initial SP value     */
   unsigned short e_csum;     /* checksum          */
   unsigned short e_ip;    /* initial IP value     */
   unsigned short e_cs;    /* initial CS value     */ 
   unsigned short e_lfarlc;   /* file address of reloc table   */
   unsigned short e_ovno;     /* overlay number       */
   unsigned short e_res1[4];  /* reserved          */
   unsigned short e_oemid;    /* OEM identifier       */
   unsigned short e_oeminfo;  /* OEM information      */
   unsigned short e_res2[10]; /* reserved          */
   unsigned long  e_lfanew;   /* file address of new header    */
   };
/**
*
* Structure of DOS relocation items
*
*/
struct REL
   {
   unsigned short offset;
   unsigned short segment;
   };
/**/
/**
*
* New executable header, used by OS/2 
*
*/
struct EXE_OS2
   {
   unsigned short ne_magic;   /* magic number 0x454E  */
   unsigned char ne_ver;      /* version number    */
   unsigned char ne_rev;      /* revision number      */
   unsigned short ne_enttab;  /* offset of entry table*/
   unsigned short ne_cbenttab;   /* # of bytes in entry table*/
   long ne_crc;            /* checksum          */
   unsigned short ne_flags;   /* miscellaneous flags     */
   unsigned short ne_autodata;   /* auto data segment number   */
   unsigned short ne_heap;    /* initial heap allocation */
   unsigned short ne_stack;   /* initial stack allocation   */
   unsigned short ne_ip;      /* initial IP offset    */
   unsigned short ne_cs;      /* initial CS segment number*/
   unsigned short ne_sp;      /* initial SP offset    */
   unsigned short ne_ss;      /* initial SS segment number*/
   unsigned short ne_cseg;    /* number of segments      */
   unsigned short ne_cmod;    /* number of module references   */
   unsigned short ne_cbnrestab;  /* size of non-resident names*/
   unsigned short ne_segtab;  /* segment table offset    */
   unsigned short ne_rsrctab; /* resource table offset   */
   unsigned short ne_restab;  /* resident name table offset */
   unsigned short ne_modtab;  /* module reference table offset*/
   unsigned short ne_imptab;  /* import name table offset   */
   unsigned long ne_nrestab;  /* non-resident name tab offset  */
   unsigned short ne_cmovent; /* number of movable entries  */
   unsigned short ne_align;   /* segment alignment shift count*/
   unsigned short ne_cres;    /* number of resource entries */
   char resv[10];          /* reserved       */
   };
/**
 *
 * Definition of bits in ne_flags
 *
 */
#define NENOTP 0x8000      /* not a process        */
#define NEIERR 0x2000      /* errors in image         */
#define NEFLTP 0x0080      /* floating point instructions */
#define NEI386 0x0040      /* 80386 instructions      */
#define NEI286 0x0020      /* 80286 instructions      */
#define NEI086 0x0010      /* 8086 instructions       */
#define NEPROT 0x0008      /* protected mode only     */
#define NEPPLI 0x0004      /* per-process library initialization*/
#define NEINST 0x0002      /* per-instance data       */
#define NESOLO 0x0001      /* solo data            */
/**
 *
 * Structure of segment table entries
 *
 */
struct SEG
   {
   unsigned short ns_sector;  /* starting sector      */
   unsigned short ns_cbseg;   /* segment size (in file)*/
   unsigned short ns_flags;   /* segment flags     */
   unsigned short ns_minalloc;   /* minimum size in bytes*/
   };
/**
*
* Definition of bits in ns_flags
*
*/
#define NSTYPE 0x0007      /* segment type mask, types are...  */
#define NSCODE 0        /*    0 for code segment   */
#define NSDATA 1        /*    1 for data segment   */
#define NSITER 0x0008      /* iterated segment        */
#define NSMOVE 0x0010      /* movable segment         */
#define NSPURE 0x0020      /* pure segment            */
#define NSPRELOAD 0x0040   /* preloaded segment    */
#define NSEXRD 0x0080      /* code segment: execute only*/
                     /* data segment: read only */
#define NSRELOC 0x0100     /* relocation information present   */
#define NSCONFORM 0x0200   /* conforming segment      */
#define NSDPL 0x0c00    /* 80286 DPL bits       */
#define SHIFTDPL 10        /* shift factor for NSDPL  */
#define NSDISCARD 0x1000   /* discardable segment     */
#define NS32BIT 0x2000     /* 32-bit segment       */
#define NSHUGE 0x4000      /* huge memory segment     */
/**/
/**
*
* name      main -- main program
*
* synopsis  main(argc,argv)
*     int argc;      number of arguments
*     char *argv[];     argument pointer array
*
* description:
* This is the main program that displays EXE file information.
*     Upon entry, argc should be 2, and argv[1] should point to
*     the EXE file name, which must include the .EXE extension.
*
*     The program sends its output to stdout, which can be
*     redirected to somewhere other than the system console.
*     Error output is sent to stderr, and when an error occurs
*     the program exits with a completion code of 1.
**/
main(argc,argv)
int argc;
char *argv[];
{
FILE *fp;
unsigned char *p;
int i,j,k;
union 
   {
   short w;
   char b[2];
   } word;
struct EXE_DOS oldexe;
struct REL rel;
struct EXE_OS2 newexe;
struct SEG seg;
char b[256];
/*
*
* Check file argument, open file, and read old EXE header
*
*/
if(argc < 2)
   {
   fprintf(stderr,"No EXE file specified\n");
   exit(1);
   }
fp = fopen(argv[1],"rb");
if(fp == NULL)
   {
   fprintf(stderr,"Can't open \"%s\"\n",argv[1]);
   exit(1);
   }
if(fread((char *)(&oldexe),sizeof(struct EXE_DOS),1,fp) != 1)
   {
   fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
   exit(1);
   }
if(oldexe.e_magic != 0x5a4d)
   {
   fprintf(stderr,"\"%s\" is not an EXE file\n",argv[1]);
   exit(1);
   }
/*
*
* Print DOS header information
*
*/
cls();
printf("DOS EXE header information for \"%s\"...\n\n",argv[1]);
printf("%40s: %u\n","Number of bytes in header",oldexe.e_cparhdr*16);
printf("%40s: %u\n","Number of 512-byte pages",oldexe.e_cp);
printf("%40s: %u\n","Number of bytes in last page",oldexe.e_cblp);
printf("%40s: %04XH\n","Checksum",oldexe.e_csum);
printf("%40s: %04XH\n","Minimum number of extra paragraphs",
        oldexe.e_minalloc);
printf("%40s: %04XH\n","Maximum number of extra paragraphs",
        oldexe.e_maxalloc);
printf("%40s: %04X:%04X\n","Starting address",oldexe.e_cs,
        oldexe.e_ip);
printf("%40s: %04X:%04X\n","Stack address",oldexe.e_ss,
        oldexe.e_sp);
printf("%40s: %u\n","Overlay number",oldexe.e_ovno);
printf("%40s: %04XH\n","OEM identifier",oldexe.e_oemid);
printf("%40s: %04XH\n","OEM information",oldexe.e_oeminfo);
printf("%40s: %u\n","Number of relocations",oldexe.e_crlc);
/*
*
* Print DOS relocation table
*
*/
if(oldexe.e_crlc)
   {
   pause(fp);
   printf("\n\nRelocation table at file offset %04XH...\n",
      oldexe.e_lfarlc);
   if(fseek(fp,(long)oldexe.e_lfarlc,0))
      {
      fprintf(stderr,"Can't seek to %04XH in \"%s\"\n",
         oldexe.e_lfarlc,argv[1]);
      exit(1);
      }
   for(i = 1; i <= oldexe.e_crlc; i++)
      {
      if(fread((char *)(&rel),sizeof(struct REL),1,fp) != 1)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      printf("%04X:%04X  ",rel.segment,rel.offset);
      if((i % 6) == 0) printf("\n");
      }
   }
/*
 *
 * Check if OS/2 executable, and terminate if not.  
 * Otherwise, read in the OS/2 header.
 *
 */
if(oldexe.e_lfanew == 0) exit(0);
pause();
printf("\n\nOS/2 EXE header information at %04XH...\n\n",
        oldexe.e_lfanew);
if(fseek(fp,(long)oldexe.e_lfanew,0))
   {
   fprintf(stderr,"Can't seek to %04XH in \"%s\"\n",
      oldexe.e_lfanew,argv[1]);
   exit(1);
   }
if(fread((char *)(&newexe),sizeof(struct EXE_OS2),1,fp) != 1)
   {
   fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
   exit(1);
   }
if(newexe.ne_magic != 0x454e)
   {
   fprintf(stderr,"\"%s\" is not an OS/2 EXE file\n",argv[1]);
   exit(1);
   }
/*
*
* Print basic information from new EXE header
*
*/
printf("%40s: %08lXH\n","Checksum",newexe.ne_crc);
printf("%40s: %u\n","Version number",newexe.ne_ver);
printf("%40s: %u\n","Revision number",newexe.ne_rev);
printf("%40s: %u\n","Number of segments",newexe.ne_cseg);
printf("%40s: %04X:%04X\n","Starting address",
        newexe.ne_cs,newexe.ne_ip);
printf("%40s: %04X:%04X\n","Stack address",newexe.ne_ss,newexe.ne_sp);
printf("%40s: %u\n","Automatic data segment number",
        newexe.ne_autodata);
printf("%40s: %04XH\n","Initial heap allocation in auto data",
        newexe.ne_heap);
printf("%40s: %04XH\n","Initial stack allocation in auto data",
        newexe.ne_stack);
printf("%40s: %04XH\n","Flags",newexe.ne_flags);
printf("%40s? %s\n","Program or library",
   (newexe.ne_flags & NENOTP) ? "LIBRARY" : "PROGRAM");
printf("%40s? %s\n","Errors in image",
   (newexe.ne_flags & NEIERR) ? "YES" : "NO");
printf("%40s? %s\n","Floating point instructions", 
   (newexe.ne_flags & NEFLTP) ? "YES" : "NO");
printf("%40s? %s\n","80386 instructions",
   (newexe.ne_flags & NEI386) ? "YES" : "NO");
printf("%40s? %s\n","80286 instructions",
   (newexe.ne_flags & NEI286) ? "YES" : "NO");
printf("%40s? %s\n","8086 instructions",
   (newexe.ne_flags & NEI086) ? "YES" : "NO");
printf("%40s? %s\n","Protected mode only",
   (newexe.ne_flags & NEPROT) ? "YES" : "NO");
printf("%40s? %s\n","Per-process initialization",
   (newexe.ne_flags & NEPPLI) ? "YES" : "NO");
printf("%40s? %s\n","Per-instance data",
   (newexe.ne_flags & NEINST) ? "YES" : "NO");
printf("%40s? %s\n","Solo data",
   (newexe.ne_flags & NESOLO) ? "YES" : "NO");
pause();
/*
*
* Print segment table
*
*/
printf("\n\nSegment table at %08lXH (%08lXH + %04XH)...\n\n",
      (long)(oldexe.e_lfanew+newexe.ne_segtab),
      oldexe.e_lfanew,newexe.ne_segtab);
printf("     FILE  FILE         MEMORY\n");
printf("SEG  ADDR  SIZE  FLAGS   SIZE\n\n");
if(fseek(fp,(long)(oldexe.e_lfanew + newexe.ne_segtab),0))
   {
   fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
      (long)(oldexe.e_lfanew+newexe.ne_segtab),argv[1]);
   exit(1);
   }
for(i = 1; i <= newexe.ne_cseg; i++)
   {
   if(fread((char *)(&seg),sizeof(struct SEG),1,fp) != 1)
      {
      fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
      exit(1);
      }
   printf("%3d  %04X  %04X  %04X    %04X  %s\n",i,
      seg.ns_sector,seg.ns_cbseg,seg.ns_flags,seg.ns_minalloc,
      (seg.ns_flags & NSTYPE) ? "Data" : "Code");
   }
pause();
/*
*
* Print imported name table
*
*/
if(newexe.ne_imptab)
   {
   printf("\n\n");
   printf("Imported Name Table at %08lXH (%08lXH + %04XH)...\n\n",
          (long)(oldexe.e_lfanew+newexe.ne_imptab),
          oldexe.e_lfanew,newexe.ne_imptab);
   if(fseek(fp,(long)(oldexe.e_lfanew + newexe.ne_imptab + 1),0))
      {
      fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
         (long)(oldexe.e_lfanew+newexe.ne_imptab + 1),argv[1]);
      exit(1);
      }
   printf("INDEX  NAME\n");
   printf("-----  ----\n");
   for(i = newexe.ne_imptab+1; i < newexe.ne_enttab; i += j+1)
      {
      j = fgetc(fp);
      if(j == EOF)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      if(j == 0) break;
      if(fread(b,j,1,fp) != 1)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      b[j] = '\0';
      printf(" %04x  %s\n",(i-newexe.ne_imptab),b);
      }
   pause();
   }
/*
*
* Print Module Reference Table
*
*/
if(newexe.ne_cmod)
   {
   printf("\n\n");
   printf("Module Reference Table at %08lXH (%08lXH + %04XH)...\n\n",
         (long)(oldexe.e_lfanew+newexe.ne_modtab),
         oldexe.e_lfanew,newexe.ne_modtab);
   if(fseek(fp,(long)(oldexe.e_lfanew + newexe.ne_modtab),0))
      {
      fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
         (long)(oldexe.e_lfanew+newexe.ne_modtab),argv[1]);
      exit(1);
      }
   printf("MODULE  INDEX\n");
   printf("------  -----\n");
   for(i = 1; i <= newexe.ne_cmod; i++)
      {
      if(fread((char *)(&j),2,1,fp) != 1)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      printf(" %04X    %04X\n",i,j);
      }
   pause();
   }  
/*
*
* Print Resident Name Table
*
*/
if(newexe.ne_restab)
   {
   printf("\n\n");
   printf("Resident Name Table at %08lXH (%08lXH + %04XH)...\n\n",
          (long)(oldexe.e_lfanew+newexe.ne_restab),
          oldexe.e_lfanew,newexe.ne_restab);
   if(fseek(fp,(long)(oldexe.e_lfanew + newexe.ne_restab),0))
      {
      fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
         (long)(oldexe.e_lfanew+newexe.ne_restab),argv[1]);
      exit(1);
      }
   printf("ORDINAL  NAME\n");
   printf("-------  ----\n");
   while(i = fgetc(fp))
      {
      if((i == EOF) || (fread(b,i,1,fp) != 1))
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      b[i] = '\0';
      if(fread((char *)(&j),2,1,fp) != 1)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      printf("   %04X  %s\n",j,b);
      }
   pause();
   }
/*
*
* Print Non-resident Name Table
*
*/
if(newexe.ne_nrestab)
   {
   printf("\n\nNon-resident Name Table at %08lXH...\n\n",
      newexe.ne_nrestab);
   if(fseek(fp,newexe.ne_nrestab,0))
      {
      fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
         newexe.ne_nrestab,argv[1]);
      exit(1);
      }
   printf("ORDINAL  NAME\n");
   printf("-------  ----\n");
   while(i = fgetc(fp))
      {
      if((i == EOF) || (fread(b,i,1,fp) != 1))
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      b[i] = '\0';
      if(fread((char *)(&j),2,1,fp) != 1)
         {
         fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
         exit(1);
         }
      printf("   %04X  %s\n",j,b);
      }
   pause();
   }
/*
*
* Print entry table
*
*/
if(newexe.ne_cbenttab)
   {
   printf("\n\nEntry table at %08lXH (%08lXH + %04XH)...\n\n",
         (long)(oldexe.e_lfanew+newexe.ne_enttab),
         oldexe.e_lfanew,newexe.ne_enttab);
   if(fseek(fp,(long)(oldexe.e_lfanew + newexe.ne_enttab),0))
      {
      fprintf(stderr,"Can't seek to %08lXH in \"%s\"\n",
             (long)(oldexe.e_lfanew+newexe.ne_enttab),argv[1]);
      exit(1);
      }
   p = malloc(newexe.ne_cbenttab);
   if(p == NULL)
      {
      fprintf(stderr,"Out of memory\n");
      exit(1);
      }
   if(fread(p,newexe.ne_cbenttab,1,fp) != 1)
      {
      fprintf(stderr,"Can't read \"%s\"\n",argv[1]);
      exit(1);
      }
   printf("SEG  OFFSET  FLAGS\n");
   printf("---  ------  -----\n");
   for(i = 0; i < newexe.ne_cbenttab; )
      {
      j = p[i++];
      k = p[i++];
      if(k) while(j-- > 0)
         {
         if(k != 255)
            {
            word.b[0] = p[i+1];
            word.b[1] = p[i+2];
            printf("%3d  %04X    %02X\n",k,word.w,p[i]);
            i += 3;
            }
         else
            {
            word.b[0] = p[i+4];
            word.b[1] = p[i+5];
            printf("%3d  %04X    %02X\n",p[i+3],word.w,p[i]);
            i += 6;
            }
         }
      }
   }
}
/**/
/**
* 
* name      pause
*
* synopsis  pause();
*
* description  This function checks if the standard output file 
*     is the system console.  If so, it displays a pause message
*     and then waits until the user presses a key.
*
**/
void pause()
{
char reverse = 0x70;
static char prompt[] = "Press any key to continue";

if(isatty(fileno(stdout)))
   {
   VioWrtNAttr((char far *)(&reverse),80,24,0,0);
   VioWrtCharStr((char far *)prompt,sizeof(prompt)-1,24,0,0);
   getch();
   cls();
   }
}
/**
*
* name      cls -- clear the screen
*
* synopsis  cls();
*
* description  This function clears the screen by calling the
*     VioScrollUp function in a special way.  However,
*     it has not be generalized to work with display
*     modes using more than 25 lines of 80 characters each.
*
**/
void cls()
{
short fill = 0x0720;

VioScrollUp(0,0,24,79,-1,(short far *)(&fill),0);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            