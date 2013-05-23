/*************************************************************************

   Program:    
   File:       ReadWholePDB.c
   
   Version:    V1.0
   Date:       30.05.02
   Function:   
   
   Copyright:  (c) Dr. Andrew C. R. Martin, University of Reading, 2002
   Author:     Dr. Andrew C. R. Martin
   Phone:      +44 (0) 1372 275775
   EMail:      andrew@bioinf.org.uk
               
**************************************************************************

   This program is not in the public domain, but it may be copied
   according to the conditions laid out in the accompanying file
   COPYING.DOC

   The code may not be sold commercially or included as part of a 
   commercial product except as described in the file COPYING.DOC.

**************************************************************************

   Description:
   ============

**************************************************************************

   Usage:
   ======

**************************************************************************

   Revision History:
   =================
   V1.0  30.05.02 Original

*************************************************************************/
/* Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "macros.h"
#include "general.h"
#include "pdb.h"


/************************************************************************/
/* Defines and macros
*/
#define MAXBUFF 160


/************************************************************************/
/* Globals
*/

/************************************************************************/
/* Prototypes
*/

/************************************************************************/
/*>void FreeWholePDB(WHOLEPDB *wpdb)
   ---------------------------------
   Input:     WHOLEPDB    *wpdb    WHOLEPDB structure to be freed

   Frees the header, trailer and atom content from a WHOLEPDB structure

   30.05.02  Original   By: ACRM
*/
void FreeWholePDB(WHOLEPDB *wpdb)
{
   FreeStringList(wpdb->header);
   FreeStringList(wpdb->trailer);
   FREELIST(wpdb->pdb, PDB);
   free(wpdb);
}

/************************************************************************/
/*>void WriteWholePDB(FILE *fp, WHOLEPDB *wpdb)
   --------------------------------------------
   Input:     FILE       *fp        File pointer
              WHOLEPDB   *wpdb      Whole PDB structure pointer

   Writes a PDB file including header and trailer information

   30.05.02  Original   By: ACRM
*/
void WriteWholePDB(FILE *fp, WHOLEPDB *wpdb)
{
   WriteWholePDBHeader(fp, wpdb);
   WritePDB(fp, wpdb->pdb);
   WriteWholePDBTrailer(fp, wpdb);
}


/************************************************************************/
/*>void WriteWholePDBHeader(FILE *fp, WHOLEPDB *wpdb)
   --------------------------------------------------
   Input:     FILE       *fp        File pointer
              WHOLEPDB   *wpdb      Whole PDB structure pointer

   Writes the header of a PDB file 

   30.05.02  Original   By: ACRM
*/
void WriteWholePDBHeader(FILE *fp, WHOLEPDB *wpdb)
{
   STRINGLIST *s;
   
   for(s=wpdb->header; s!=NULL; NEXT(s))
   {
      fputs(s->string, fp);
   }
}


/************************************************************************/
/*>void WriteWholePDBTrailer(FILE *fp, WHOLEPDB *wpdb)
   ---------------------------------------------------
   Input:     FILE       *fp        File pointer
              WHOLEPDB   *wpdb      Whole PDB structure pointer

   Writes the trailer of a PDB file 

   30.05.02  Original   By: ACRM
*/
void WriteWholePDBTrailer(FILE *fp, WHOLEPDB *wpdb)
{
   STRINGLIST *s;
   
   for(s=wpdb->trailer; s!=NULL; NEXT(s))
   {
      fputs(s->string, fp);
   }
}


/************************************************************************/
/*>WHOLEPDB *ReadWholePDB(FILE *fpin)
   ----------------------------------
   Input:     FILE      *fpin     File pointer
   Returns:   WHOLEPDB  *         Whole PDB structure containing linked
                                  list to PDB coordinate data

   Reads a PDB file, storing the header and trailer information as
   well as the coordinate data. Can read gzipped files as well as
   uncompressed files.

   Coordinate data is accessed as linked list of type PDB as follows:
   
   WHOLEPDB *wpdb;
   PDB      *p;
   wpdb = ReadWholePDB(fp);
   for(p=wpdb->pdb; p!=NULL; p=p->next)
   {
      ... Do something with p ...
   }

   30.05.02  Original   By: ACRM
*/
WHOLEPDB *ReadWholePDB(FILE *fpin)
{
   WHOLEPDB *wpdb;
   char     buffer[MAXBUFF];
   FILE     *fp = fpin;
   
#ifdef GUNZIP_SUPPORT
   int      signature[3],
            i,
            ch;
   char     cmd[80];
#endif

   if((wpdb=(WHOLEPDB *)malloc(sizeof(WHOLEPDB)))==NULL)
      return(NULL);

   wpdb->pdb     = NULL;
   wpdb->header  = NULL;
   wpdb->trailer = NULL;
   
#ifdef GUNZIP_SUPPORT
   cmd[0] = '\0';
   
   /* See whether this is a gzipped file                                */
   for(i=0; i<3; i++)
      signature[i] = fgetc(fpin);
   for(i=2; i>=0; i--)
      ungetc(signature[i], fpin);
   if((signature[0] == (int)0x1F) &&
      (signature[1] == (int)0x8B) &&
      (signature[2] == (int)0x08))
   {
      /* It is gzipped so we'll open gunzip as a pipe and send the data
         through that into a temporary file
      */
      cmd[0] = '\0';
      sprintf(cmd,"gunzip >/tmp/readpdb_%d",(int)getpid());
      if((fp = (FILE *)popen(cmd,"w"))==NULL)
      {
         wpdb->natoms = (-1);
         return(NULL);
      }
      while((ch=fgetc(fpin))!=EOF)
         fputc(ch, fp);
      pclose(fp);

      /* We now reopen the temporary file as our PDB input file         */
      sprintf(cmd,"/tmp/readpdb_%d",(int)getpid());
      if((fp = fopen(cmd,"r"))==NULL)
      {
         wpdb->natoms = (-1);
         return(NULL);
      }
   }
#endif   

   /* Read the header from the PDB file                                 */
   while(fgets(buffer,MAXBUFF,fp))
   {
      if(!strncmp(buffer, "ATOM  ", 6) ||
         !strncmp(buffer, "HETATM", 6) ||
         !strncmp(buffer, "MODEL ", 6))
      {
         break;
      }
      if((wpdb->header = StoreString(wpdb->header, buffer))==NULL)
         return(NULL);
   }
   
   /* Read the coordinates                                              */
   rewind(fp);
   wpdb->pdb = ReadPDB(fp, &(wpdb->natoms));

   /* Read the trailer                                                  */
   rewind(fp);
   while(fgets(buffer,MAXBUFF,fp))
   {
      if(!strncmp(buffer, "CONECT", 6) ||
         !strncmp(buffer, "MASTER", 6) ||
         !strncmp(buffer, "END   ", 6))
      {
         wpdb->trailer = StoreString(wpdb->trailer, buffer);
      }
   }
   
   return(wpdb);
}
