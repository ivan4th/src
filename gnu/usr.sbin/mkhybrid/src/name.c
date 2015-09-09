/*
 * File name.c - map full Unix file names to unique 8.3 names that
 * would be valid on DOS.
 *

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "config.h"
#include "mkisofs.h"

#include <ctype.h>

extern int allow_leading_dots;
   
/*
 * Function:	iso9660_file_length
 *
 * Purpose:	Map file name to 8.3 format, return length
 *		of result.
 *
 * Arguments:	name	file name we need to map.
 *		sresult	directory entry structure to contain mapped name.
 *		dirflag	flag indicating whether this is a directory or not.
 *
 * Notes:	This procedure probably needs to be rationalized somehow.
 *		New options to affect the behavior of this function
 *		would also be nice to have.
 */
int FDECL3(iso9660_file_length,
	   const char*, name, 
	   struct directory_entry *, sresult, 
	   int, dirflag)
{
  char		* c;
  int		  chars_after_dot  = 0;
  int		  chars_before_dot = 0;
  int		  current_length   = 0;
  int		  extra		   = 0;
  int		  ignore	   = 0;
  char		* last_dot;
  const char	* pnt;
  int		  priority	   = 32767;
  char		* result;
  int		  seen_dot	   = 0;
  int		  seen_semic	   = 0;
  int		  tildes	   = 0;

  result = sresult->isorec.name;

  /*
   * For the '.' entry, generate the correct record, and return
   * 1 for the length.
   */
  if(strcmp(name,".") == 0)
    {
      if(result) 
	{
	  *result = 0;
	}
      return 1;
    }

  /*
   * For the '..' entry, generate the correct record, and return
   * 1 for the length.
   */
  if(strcmp(name,"..") == 0)
    {
      if(result) 
	{
	  *result++ = 1;
	  *result++ = 0;
	}
      return 1;
    }

  /*
   * Now scan the directory one character at a time, and figure out
   * what to do.
   */
  pnt = name;

  /*
   * Find the '.' that we intend to use for the extension.  Usually this
   * is the last dot, but if we have . followed by nothing or a ~, we
   * would consider this to be unsatisfactory, and we keep searching.
   */
  last_dot = strrchr (pnt,'.');
  if(    (last_dot != NULL)
      && (    (last_dot[1] == '~')
 	   || (last_dot[1] == '\0')) )
    {
      c = last_dot;
      *c = '\0';
      last_dot = strrchr (pnt,'.');
      *c = '.';
    }

  while(*pnt)
    {
#ifdef VMS
      if( strcmp(pnt,".DIR;1") == 0 ) 
	{
	  break;
	}
#endif

      /*
       * This character indicates a Unix style of backup file
       * generated by some editors.  Lower the priority of
       * the file.
       */
      if(*pnt == '#') 
	{
	  priority = 1; 
	  pnt++; 
	  continue; 
	}

      /*
       * This character indicates a Unix style of backup file
       * generated by some editors.  Lower the priority of
       * the file.
       */
      if(*pnt == '~') 
	{
	  priority = 1; 
	  tildes++; 
	  pnt++; 
	  continue;
	}

      /*
       * This might come up if we had some joker already try and put
       * iso9660 version numbers into the file names.  This would be
       * a silly thing to do on a Unix box, but we check for it
       * anyways.  If we see this, then we don't have to add our
       * own version number at the end.
       * UNLESS the ';' is part of the filename and no version
       * number is following. [VK]
       */
       if(*pnt == ';')
	 {
	   /* [VK] */
	   if (pnt[1] != '\0' && (pnt[1] < '0' || pnt[1] > '9'))
	     {
	       pnt++;
	       ignore++;
	       continue;
	     }
	 }

      /*
       * If we have a name with multiple '.' characters, we ignore everything
       * after we have gotten the extension.
       */
      if(ignore) 
	{
	  pnt++; 
	  continue;
	}

      /*
       * Spin past any iso9660 version number we might have.
       */
      if(seen_semic)
	{
	  if(*pnt >= '0' && *pnt <= '9') 
	    {
	      *result++ = *pnt;
	    }
	  extra++;
	  pnt++;
	  continue;
	}

	/*
	 * If we have full names, the names we generate will not
	 * work on a DOS machine, since they are not guaranteed
	 * to be 8.3.  Nonetheless, in many cases this is a useful
	 * option.  We still only allow one '.' character in the
	 * name, however.
	 */
      if(full_iso9660_filenames) 
	{
	  /* Here we allow a more relaxed syntax. */
	  if(*pnt == '.') 
	    {
	      if (seen_dot) 
		{
		  ignore++; 
		  continue;
		}
	      seen_dot++;
	    }
	  if(current_length < 30) 
	    {
	      if(!isascii(*pnt))
		{
		  *result++ = '_';
		}
	      else
		{
		  *result++ = (islower((unsigned char)*pnt) ? toupper((unsigned char)*pnt) : *pnt);
		}
	    }
	}
      else
	{ 
	  /* 
	   * Dos style filenames.  We really restrict the
	   * names here.
	   */
	  /* It would be nice to have .tar.gz transform to .tgz,
	   * .ps.gz to .psz, ...
	   */
	  if(*pnt == '.') 
	    {
	      if (!chars_before_dot && !allow_leading_dots) 
		{
		  /* DOS can't read files with dot first */
		  chars_before_dot++;
		  if (result) 
		    {
		      *result++ = '_'; /* Substitute underscore */
		    }
		}
	      else if( pnt != last_dot )
		{
		  /*
		   * If this isn't the dot that we use for the extension,
		   * then change the character into a '_' instead.
		   */
		  if(chars_before_dot < 8) 
		    {
		      chars_before_dot++;
		      if(result) 
			{
			  *result++ = '_';
			}
		    }
		}
	      else 
		{
		  if (seen_dot) 
		    {
		      ignore++; continue;
		    }
		  if(result) 
		    {
		      *result++ = '.';
		    }
		  seen_dot++;
		}
	    }
	  else 
	    {
	      if(    (seen_dot && (chars_after_dot < 3) && ++chars_after_dot)
		     || (!seen_dot && (chars_before_dot < 8) && ++chars_before_dot) )
		{
		  if(result) 
		    {
		      switch (*pnt) 
			{
			default:
			  if(!isascii(*pnt))
			    {
			      *result++ = '_';
			    }
			  else
			    {
			      *result++ = islower((unsigned char)*pnt) ? toupper((unsigned char)*pnt) : *pnt;
			    }
			  break;

			/* 
			 * Descriptions of DOS's 'Parse Filename'
			 * (function 29H) describes V1 and V2.0+
			 * separator and terminator characters.
			 * These characters in a DOS name make
			 * the file visible but un-manipulable
			 * (all useful operations error off.
			 */
			/* separators */
			case '+':
			case '=':
			case '%': /* not legal DOS filename */
			case ':':
			case ';': /* already handled */
			case '.': /* already handled */
			case ',': /* already handled */
			case '\t':
			case ' ':
			  /* V1 only separators */
			case '/':
			case '"':
			case '[':
			case ']':
			  /* terminators */
			case '>':
			case '<':
			case '|':
			  /* Hmm - what to do here?  Skip?
			   * Win95 looks like it substitutes '_'
			   */
			  *result++ = '_';
			  break;
			} /* switch (*pnt) */
		    } /* if (result) */
		} /* if (chars_{after,before}_dot) ... */
	    } /* else *pnt == '.' */
	} /* else DOS file names */
      current_length++;
      pnt++;
    } /* while (*pnt) */
  
  /*
   * OK, that wraps up the scan of the name.  Now tidy up a few other
   * things.
   */

  /*
   * Look for emacs style of numbered backups, like foo.c.~3~.  If
   * we see this, convert the version number into the priority
   * number.  In case of name conflicts, this is what would end
   * up being used as the 'extension'.
   */
  if(tildes == 2)
    {
      int prio1 = 0;
      pnt = name;
      while (*pnt && *pnt != '~') 
	{
	  pnt++;
	}
      if (*pnt) 
	{
	  pnt++;
	}
      while(*pnt && *pnt != '~')
	{
	  prio1 = 10*prio1 + *pnt - '0';
	  pnt++;
	}
      priority = prio1;
    }
  
  /*
   * If this is not a directory, force a '.' in case we haven't
   * seen one, and add a version number if we haven't seen one
   * of those either.
   */
  if (!dirflag)
    {
      if (!seen_dot && !omit_period) 
	{
	  if (result) *result++ = '.'; 
	  extra++;
	}
      if(!omit_version_number && !seen_semic) 
	{
	  if(result)
	    {
	      *result++ = ';';
	      *result++ = '1';
	    };
	  extra += 2;
	}
    }
		    
  if(result) 
    {
      *result++ = 0;
    }
  sresult->priority = priority;

  return (chars_before_dot + chars_after_dot + seen_dot + extra);
}
