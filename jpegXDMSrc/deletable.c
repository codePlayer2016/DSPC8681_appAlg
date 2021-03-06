/******************************************************************************/
/*            Copyright (c) 2006 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/******************************************************************************/

/*!
********************************************************************************
  @file     ConfigParser.c
  @brief    This file contains parsing routines for configuration file
  @author   Multimedia Codecs TI India
  @version  0.0 - Jan 24,2006    initial version
********************************************************************************
*/

/* standard C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Client header file                                                         */
#include "jpegDecoder.h"

#ifdef C6000
extern far sTokenMapping sTokenMap[] ;
#else
extern sTokenMapping sTokenMap[] ;
#endif

static XDAS_Int8 buf[20000];
//Reads the configuration file content in a buffer and returns the address 
//of the buffer
static XDAS_Int8 *GetConfigFileContent (FILE *fname)
{
  XDAS_Int32 FileSize;

  if (0 != fseek (fname, 0, SEEK_END))
  {
    return 0;
  }

  FileSize = ftell (fname);
  if (FileSize < 0 || FileSize > 20000)
  {
    return 0;

  }
  if (0 != fseek (fname, 0, SEEK_SET))
  {
    return 0;
  }

  /****************************************************************************/
  /* Note that ftell() gives us the file size as the file system sees it.     */
  /* The actual file size, as reported by fread() below will be often smaller */ 
  /* due to CR/LF to CR conversion and/or control characters after the dos    */
  /* EOF marker in the file.                                                  */
  /****************************************************************************/
  FileSize = fread (buf, 1, FileSize, fname);
  buf[FileSize] = '\0';
  fclose (fname);
  return buf;
}


/*!
 ***********************************************************************
 * \brief
 *    Returns the index number from sTokenMap[] for a given parameter name.
 * \param s
 *    parameter name string
 * \return
 *    the index number if the string is a valid parameter name,         
 *    -1 for error
 ***********************************************************************
 */
static XDAS_Int32 ParameterNameToMapIndex (XDAS_Int8 *s)
{
  XDAS_Int32 i = 0;

  while (sTokenMap[i].tokenName != NULL)
    if (0==strcmp ((const char *)sTokenMap[i].tokenName, (const char *)s))
      return i;
    else
      i++;
  return -1;
}

/*!
********************************************************************************
  @fn				XDAS_Int32 ParseContent (XDAS_Int8 *buf, 
                XDAS_Int32 bufsize)
	@brief		Parses the character array buf and writes global variable input, 
            which is defined in configfile.h.  This hack will continue to be 
            necessary to facilitate the addition of new parameters through the 
            sTokenMap[] mechanism-Need compiler-generated addresses in sTokenMap

  @param  buf[IN]     buffer to be parsed
  buffer  bufsize[IN] size of buffer
	@return							status ( PASS/ FAIL)
********************************************************************************
*/

#define MAX_ITEMS_TO_PARSE  1000

static XDAS_Int32 ParseContent (XDAS_Int8 *buf, XDAS_Int32 bufsize)
{

  XDAS_Int8 *items[MAX_ITEMS_TO_PARSE];
  XDAS_Int32 MapIdx;
  XDAS_Int32 item = 0;
  XDAS_Int32 InString = 0, InItem = 0;
  XDAS_Int8 *ptr = buf;
  XDAS_Int8 *bufend = &buf[bufsize];
  XDAS_Int32 IntContent;
  XDAS_Int32 i;
  FILE *fpErr = stderr;

// Stage one: Generate an argc/argv-type list in items[], without comments and
// whitespace.
// This is context insensitive and could be done most easily with lex(1).

	while (ptr < bufend)
	{
		switch (*ptr)
		{
			/* Procss whitespace */
		case ' ':
		case '\t':
			if (InString) /* Skip whitespace and leave state unchanged */
			{
				ptr++;
			}
			else /* Terminate non-strings if whitespace is found */
			{
				*ptr++ = '\0';
				InItem = 0;
			}
			break;

			/* Process comment */
		case '#':
			/* Replace '#' with '\0' in case of comment
			immediately following tlongeger or string */
			*ptr = '\0';
			/* Skip till EOL or EOF, whichever comes first */
			while (*ptr != '\n' && ptr < bufend)
				ptr++;
			InString = 0;
			InItem = 0;
			break;

			/* Process begin and end of a string */
		case '"':
			*ptr++ = '\0';
			if (!InString)
			{
				items[item++] = ptr;
				InItem = ~InItem;
			}
			else
			{
				InItem = 0;
			}
			/* Toggle */
			InString = ~InString;
			break;

		case '\n':
			InItem = 0;
			InString = 0;
			*ptr++='\0';
			break;

		case 13:
			ptr++;
			break;

		default:
			if (!InItem)
			{
				items[item++] = ptr;
				InItem = ~InItem;
			}
			ptr++;
		}
	}

  item--;

  for (i=0; i<item; i+= 3)
  {
    if (0 > (MapIdx = ParameterNameToMapIndex (items[i])))
    {
      fprintf(fpErr, " \nParameter Name '%s' not recognized.. ", items[i]);
      return -1 ;

    }
    if (strcmp ("=", (const char *)items[i+1]))
    {
      fprintf(fpErr, " \nfile: '=' expected as the second token in each line.");
      return -1 ;
    }
    sscanf ((const char *)items[i+2], "%d", &IntContent) ;
    * ((XDAS_Int32 *) (sTokenMap[MapIdx].place)) = IntContent;
  }
  return 0 ;
}

XDAS_Int32 readparamfile(FILE * fname)
{
  XDAS_Int8 *FileBuffer = NULL ;
  XDAS_Int32 retVal ; 

  //read the content in a buffer
  FileBuffer = GetConfigFileContent(fname);

  if(FileBuffer)
  {
    retVal  = ParseContent(FileBuffer,strlen((const char *)FileBuffer));
    return retVal ;
  }
  else
    return -1;
}
/******************************************************************************/
/*            Copyright (c) 2006 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/******************************************************************************/
