/* A simple front end debugger.
   Copyright (C) 2012 Kenneth Olsson

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/** @file vsscanner.c
 *
 * @brief A very simple source code scanner.
 *
 * The scanner consists of two types of scanner groups, @a start and @a normal.
 * The start group matches beginning of a line, '^'. The normal group all other.
 *
 * The start and normal group consist of a set of @a scan_match:es. Each
 * @a scan_match has an @a id to identify the match and a parts that build
 * up the match. E.g a match '^[A-B]*CDE'.
 *
 * @code
 * scanner [vsscanner]:
 *   start [scan_group]:
 *     matches [scan_match]:
 *	 first [part_match]:
 *	   match 'AB'
 *	   type zero_or_more
 *	   next [part_match]
 *	     match 'CDE'
 *	     type exact.
 *	     next = NULL.
 *     matches...
 * @endcode
 *
 * Support for:
 * - [ ] and [^ ]
 * - [:lower:], [:upper:], [:digit:], [:alnum:], [:punct:]  [:graph:] and
 *   [:xdigit:]
 * - '\\?', '\\+' and '*'
 *
 * @todo '.', \*.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "vsscanner.h"
#include "debug.h"

#define SCAN_INCREASE 10

/**
 * @name Match types.
 *
 * Defines match group types, such as ':alpha:', ':upper:', etc.
 */
/**{*/
#define PART_TYPE_GROUP_LOWER   0x01
#define PART_TYPE_GROUP_UPPER   0x02
#define PART_TYPE_GROUP_DIGIT   0x04
#define PART_TYPE_GROUP_ALNUM   0x08
#define PART_TYPE_GROUP_GRAPH   0x10
#define PART_TYPE_GROUP_PUNCT   0x20
#define PART_TYPE_GROUP_XDIGIT  0x40
/**}*/

/**
 * @name Is functions
 *
 * Test a if a string begins with specified char(s).
 */
/**{*/
#define IS_ONE_OR_MORE(x) ((x)[0] == '\\' && (x)[1] == '+')
#define IS_ZERO_OR_ONE(x) ((x)[0] == '\\' && (x)[1] == '?')
#define IS_ZERO_OR_MORE(x) ((x)[0] == '*')
#define IS_ANY(x) ((*x) == '.')

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/

/**
 * @brief Defines a part of a match.
 *
 * Defines a part of a match. E.g. in "abc[EF]+gh" would have the parts "ab",
 * "EF" and gh.
 */
typedef struct part_match_t
{
  int type;	       /**< Type of match. */
  int is_any;	       /**< 1 if all match, '.' */
  char *match;	       /**<
			* The match, if needed. E.g. [:alpha:] does not need
			* it.
			*/
  char match_single;   /**< Match a single char.*/
  int mult;	       /**< The multiplier, '?', '+' and '*'. */
  int group;	       /**<
			* If the match belongs to a group such as ':alnum:'. See
			* Match types
			*/

  int hits;			 /**< Number of match hits so far. */
  struct part_match_t *next;	 /**< The next part match. */
} part_match;

/**
 * @brief Defines a match.
 *
 * Defines a single match, e.g. "abc[def]+gh".
 */
typedef struct scan_match_t
{
  int id;		 /**< The id that is reported if match is found. */

  part_match *first;	 /**< The first part of the expression. */
  part_match *current;	 /**<
			  * Current part while scanning, used if match is more
			  * than a line.
			  */
  int multiline;   /**< 1 if match over multiple lines. */
  int word;	   /**<
		    * 1 if the match is a word, compare to \\bcar\\b. E.g.
		    * if match is 'car' neither 'scar' or 'card' with match,
		    * but 'car!' will match.
		    */
} scan_match;

/**
 * @brief A group of matches.
 *
 * A set of defined matches.
 */
typedef struct scan_group_t
{
  int type;		  /**< Type of match. */
  scan_match *matches;	  /**< An array of matches. */

  int size;    /**< Available size of the array. */
  int len;     /**< Used matches in the array. */
} scan_group;

/**
 * @brief The scanner.
 *
 * Defines the scanner.
 */
struct vsscanner_t
{
  scan_group start;	/**<
			 * A group of matchings at beginning of line,
			 * '^abc'.
			 */
  scan_group normal;	/**< Normal matches. */

  scan_match *current_match;   /**<
				* Current match, if match spans over several
				* lines.
				*/

  int state;   /**< Current state of the parser. */
};

enum
{
  STATE_NONE = -1,
  STATE_START,
  STATE_NORMAL,
  STATE_MATCH,
};

enum
{
  TYPE_BEGINNING,
  TYPE_NORMAL,
};

enum
{
  PART_TYPE_NONE_OF,
  PART_TYPE_ONE_OF,
  PART_TYPE_EXACT,
  PART_TYPE_EXACT_SINGLE,
  PART_TYPE_ANY,
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
int id_table_add (id_table * idt, int id, int index, int len);
int part_match_match (part_match * pm, const char *text);
const char *part_match_inner (part_match * pm, const char *start);
const char *part_match_exact (part_match * pm, const char *start);
part_match *part_match_create (const char *start);

int vsscanner_scan_match (vsscanner * scanner, scan_match * sm,
			  const char *text, int *ind, id_table * idt);


/**
 * @brief Add id and index to the table.
 *
 * Adds a index and a id to a table. If more room is needed, it will be
 * allocated.
 *
 * @param idt The table.
 * @param id The id.
 * @param index The index, or column.
 * @param len Length of the toke.
 *
 * @return 0 upon success.
 */
int
id_table_add (id_table * idt, int id, int index, int len)
{
  id_entry *t;

  assert (idt);

  if (idt->len == 1 && index == 0)
    {
      idt->len = 0;
    }

  DINFO (10, "Add %d %d %d to table", id, index, len);
  if (idt->len >= idt->size)
    {
      idt->size += DEF_IDT_LEN;
      t = (id_entry *) realloc (idt->extra_id,
				(idt->size -
				 DEF_IDT_LEN) * sizeof (id_entry));
      if (t == NULL)
	{
	  return -1;
	}
      idt->extra_id = t;
    }

  if (idt->len < DEF_IDT_LEN)
    {
      idt->id[idt->len].id = id;
      idt->id[idt->len].index = index;
      idt->id[idt->len].len = len;
    }
  else
    {
      idt->extra_id[idt->len - DEF_IDT_LEN].id = id;
      idt->extra_id[idt->len - DEF_IDT_LEN].index = index;
      idt->extra_id[idt->len - DEF_IDT_LEN].len = len;
    }
  idt->len++;
  return 0;
}

/**
 * @brief Test if a part is a match.
 *
 * Checks if a part matches a text.
 *
 * @param pm The part.
 * @param text The text.
 *
 * @return 0 if the part matches the text. -1 if the part does not match.
 */
int
part_match_match (part_match * pm, const char *text)
{
  int match;

  match = 0;
  switch (pm->type)
    {
    case PART_TYPE_NONE_OF:
      if (pm->is_any)
	{
	  break;
	}
      match = 1;
      if ((pm->group & PART_TYPE_GROUP_LOWER) && islower (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_UPPER) && isupper (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_DIGIT) && isdigit (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_ALNUM) && isalnum (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_GRAPH) && isgraph (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_PUNCT) && ispunct (*text))
	{
	  match = 0;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_XDIGIT) && isxdigit (*text))
	{
	  match = 0;
	  break;
	}
      if (pm->match && strchr (pm->match, *text))
	{
	  match = 0;
	  break;
	}
      break;
    case PART_TYPE_ONE_OF:
      if (pm->is_any)
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_LOWER) && islower (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_UPPER) && isupper (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_DIGIT) && isdigit (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_ALNUM) && isalnum (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_GRAPH) && isgraph (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_PUNCT) && ispunct (*text))
	{
	  match = 1;
	  break;
	}
      if ((pm->group & PART_TYPE_GROUP_XDIGIT) && isxdigit (*text))
	{
	  match = 1;
	  break;
	}
      if (pm->match && strchr (pm->match, *text) != NULL)
	{
	  match = 1;
	  break;
	}
      break;
    case PART_TYPE_EXACT_SINGLE:
      match = pm->is_any ? 1 : (pm->match_single == *text);
      break;
    case PART_TYPE_EXACT:
      match = strncmp (text, pm->match, strlen (pm->match)) == 0;
      break;
    }
  DINFO (10, "Match %d %s", match, text);
  return match;
}

/**
 * @brief Build a part match.
 *
 * Build a part match, that is from '[...]'
 *
 * @param pm The part to be built.
 * @param start The start of the text, e.g. 'ab' in '[ab]'.
 *
 * @return A pointer to the next part. NULL if an error is encounterd.
 */
const char *
part_match_inner (part_match * pm, const char *start)
{
  const char *q = start;
  int range;
  int len;
  char *w;
  char r;
  int is_any = 0;

  assert (pm);
  assert (start);

  /* Get length. */
  q = start;
  len = 0;
  while (*q && (*q != ']' || *(q - 1) == '\\'))
    {
      if (IS_ANY (q))
	{
	  len = 0;
	  is_any = 1;
	  pm->is_any = 1;
	  break;
	}

      if (*(q + 1) == '-')
	{
	  /* A range, e.g. 'A-Z' */
	  range = *(q + 2) - *q;
	  if (range < 0)
	    {
	      goto error;
	    }
	  len += range + 1;
	  q += 3;
	}
      else if (strncmp (q, "[:lower:]", 9) == 0
	       || strncmp (q, "[:upper:]", 9) == 0
	       || strncmp (q, "[:digit:]", 9) == 0
	       || strncmp (q, "[:alnum:]", 9) == 0
	       || strncmp (q, "[:graph:]", 9) == 0
	       || strncmp (q, "[:punct:]", 9) == 0
	       || strncmp (q, "[:alpha:]", 9) == 0)
	{
	  q += 9;
	}
      else if (strncmp (q, "[:xdigit:]", 10) == 0)
	{
	  q += 10;
	}
      else if (*q == '\\')
	{
	  len++;
	  q += 2;
	}
      else
	{
	  len++;
	  q++;
	}
    }
  if (*q == '\0')
    {
      goto error;
    }

  if (len > 0)
    {
      pm->match = (char *) malloc (len + 1);
      if (pm->match == NULL)
	{
	  goto error;
	}
    }
  else
    {
      pm->match = NULL;
    }

  /* Get the characters. */
  q = start;
  w = pm->match;
  while (*q && (*q != ']' && *(q - 1) != '\\') && !is_any)
    {
      if (*(q + 1) == '-')
	{
	  /* A range, e.g. 'A-Z' */
	  for (r = *q; r <= *(q + 2); r++)
	    {
	      *w++ = r;
	    }
	  q += 3;

	}
      else if (strncmp (q, "[:lower:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_LOWER;
	  q += 9;
	}
      else if (strncmp (q, "[:upper:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_UPPER;
	  q += 9;
	}
      else if (strncmp (q, "[:alpha:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_LOWER | PART_TYPE_GROUP_UPPER;
	  q += 9;
	}
      else if (strncmp (q, "[:digit:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_DIGIT;
	  q += 9;
	}
      else if (strncmp (q, "[:alnum:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_LOWER | PART_TYPE_GROUP_UPPER
	    | PART_TYPE_GROUP_DIGIT;
	  q += 9;
	}
      else if (strncmp (q, "[:graph:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_GRAPH;
	  q += 9;
	}
      else if (strncmp (q, "[:punct:]", 9) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_PUNCT;
	  q += 9;
	}
      else if (strncmp (q, "[:xdigit:]", 10) == 0)
	{
	  pm->group |= PART_TYPE_GROUP_XDIGIT;
	  q += 10;
	}
      else if (*q == '\\')
	{
	  q++;
	  *w = *q;
	  q++;
	  w++;
	}
      else
	{
	  *w = *q;
	  q++;
	  w++;
	}
    }
  if (is_any)
    {
      q = start;
      while (*q && (*q != ']' && *(q - 1) != '\\'))
	{
	  q++;
	}
    }

  if (len > 0)
    {
      *w = '\0';
    }
  return q + 1;

error:
  if (pm->match != NULL)
    {
      free (pm->match);
      pm->match = NULL;
    }
  return NULL;
}

/**
 * @brief Build a part.
 *
 * Constructs a normal part, e.g. 'AB' in 'ABC+'
 *
 * @param pm The part.
 * @param start The start of the text.
 *
 * @return A pointer to the next part. NULL if failed to construct the part.
 */
const char *
part_match_exact (part_match * pm, const char *start)
{
  const char *q;
  char *w;
  int len;

  assert (pm);

  if (IS_ONE_OR_MORE (start + 1) || IS_ZERO_OR_ONE (start + 1)
      || (*start != '\\' && IS_ZERO_OR_MORE (start + 1))
      || *(start + 1) == '[' || *(start + 1) == '\0')
    {
      pm->match_single = *start;
      pm->type = PART_TYPE_EXACT_SINGLE;
      pm->is_any = IS_ANY (start);
      return start + 1;
    }

  /* Get length */
  len = 0;
  q = start;
  while (*q && *q != '[' && !IS_ANY (q)
	 && (!IS_ONE_OR_MORE (q + 1) && !IS_ZERO_OR_ONE (q + 1)
	     && !(*q != '\\' && IS_ZERO_OR_MORE (q + 1))))
    {
      if (*q == '\\' && *(q + 1) != '\0')
	{
	  q += 2;
	}
      else
	{
	  q++;
	}
      len++;
    }

  pm->match = (char *) malloc (len + 1);
  /* Copy */
  q = start;
  w = pm->match;
  while (*q && *q != '[' && !IS_ANY (q)
	 && (!IS_ONE_OR_MORE (q + 1) && !IS_ZERO_OR_ONE (q + 1)
	     && !(*q != '\\' && IS_ZERO_OR_MORE (q + 1))))
    {
      if (*q == '\\' && *(q + 1) != '\0')
	{
	  *w = *(q + 1);
	  q += 2;
	}
      else
	{
	  *w = *q;
	  q++;
	}
      w++;
    }

  *w = '\0';
  pm->type = PART_TYPE_EXACT;

  DINFO (10, "new exact part '%s' (%d)", pm->match, len);
  return q;
}

/**
 * @brief Create a set of parts.
 *
 * Create a set of parts, that defines a match.
 *
 * @param text The text.
 *
 * @return A pointer to the parts.
 */
part_match *
part_match_create (const char *text)
{
  part_match *pm;
  const char *p;
  const char *q;

  DINFO (5, "Adding part '%s'", text);

  if (*text == '\0')
    {
      return NULL;
    }

  pm = (part_match *) malloc (sizeof (*pm));
  if (pm == NULL)
    {
      return NULL;
    }
  memset (pm, 0, sizeof (*pm));

  if (*text == '[')
    {
      if (*(text + 1) == '^')
	{
	  /* '[^ABC]...' */
	  pm->type = PART_TYPE_NONE_OF;
	  p = text + 2;
	}
      else
	{
	  /* '[ABC]...' */
	  pm->type = PART_TYPE_ONE_OF;
	  p = text + 1;
	}
      q = part_match_inner (pm, p);
    }
  else
    {
      q = part_match_exact (pm, text);
    }

  /* Get mult field */
  if (IS_ONE_OR_MORE (q))
    {
      pm->mult = '+';
      q += 2;
    }
  else if (IS_ZERO_OR_ONE (q))
    {
      pm->mult = '?';
      q += 2;
    }
  else if (IS_ZERO_OR_MORE (q))
    {
      pm->mult = '*';
      q++;
    }
  else
    {
      pm->mult = 0;
    }

  pm->next = part_match_create (q);
  DINFO (5, "Added part '%s' (%c)  mult: %c type %d",
	 pm->match ? pm->match : "[NaN]", pm->match_single, pm->mult,
	 pm->type);
  return pm;
}

/**
 * @brief Match a text.
 *
 * Match text to a defined match. If a match is found, the result table will
 * be updated.
 *
 * @param scanner The scanner.
 * @param sm The match to match against.
 * @param text The text.
 * @param ind Start of the text.
 * @param idt The result table.
 *
 * @return 0 if match is found. -1 if no match. -2 if beginning of match, but
 *         not the end of the match on the line.
 */
int
vsscanner_scan_match (vsscanner * scanner, scan_match * sm, const char *text,
		      int *ind, id_table * idt)
{
  int i;
  int match;
  const char *r;
  part_match *p;
  int ret = -1;
  int len;
  int hit = 0;

  if (sm->current != NULL)
    {
      p = sm->current;
    }
  else
    {
      p = sm->first;
    }

  if (sm->word)
    {
      /* A single word. */
      len = strlen (sm->first->match);
      DINFO (10, "Checking word '%s' (%d) cmp to '%s'", sm->first->match, len,
	     text);
      if ((*ind == 0 || ispunct (text[*ind - 1]) || isblank (text[*ind - 1]))
	  && (text[*ind + len] == '\0' || ispunct (text[*ind + len])
	      || isspace (text[*ind + len]))
	  && strncmp (text + *ind, sm->first->match, len) == 0)
	{
	  /* Word matches. */
	  r = text + *ind + len;
	  ret = 0;
	  goto out;
	}
      return -1;
    }

  i = *ind;
  r = &text[i];
  while (p && *r)
    {
      match = part_match_match (p, r);
      hit += match;
      DINFO (10, "Match %d '%s' (%c) to '%s' [%s] %d [%p]", match,
	     p->match ? p->match : "[NaN]", p->match_single, r, text,
	     r - text, p->next);
      switch (p->mult)
	{
	case '*':
	  /* zero or many hits */
	  /* Check if next part is a match. If so choose that one. */
	  if (p->next && part_match_match (p->next, r))
	    {
	      p->hits = 0;
	      p = p->next;
	      break;
	    }
	  if (!match)
	    {
	      /* Test with next group. */
	      p->hits = 0;
	      p = p->next;
	    }
	  else
	    {
	      p->hits++;
	      r++;
	    }
	  break;
	case '?':
	  /* zero or one hit */
	  if (!match)
	    {
	      /* Test with next group. */
	      p->hits = 0;
	      p = p->next;
	      break;
	    }
	  else
	    {
	      p->hits = 0;
	      p = p->next;
	      r++;
	    }
	  break;
	case '+':
	  /* one or more hits */
	  /* If we got one hit already, see if next part is a hit. */
	  if (p->hits > 0 && p->next && part_match_match (p->next, r))
	    {
	      p->hits = 0;
	      p = p->next;
	      break;
	    }
	  if (!match)
	    {
	      if (p->hits > 0)
		{
		  /* Test with next group. */
		  p->hits = 0;
		  p = p->next;
		}
	      else
		{
		  return -1;
		}
	    }
	  else
	    {
	      p->hits++;
	      r++;
	    }
	  break;
	case '\0':
	  if (!match)
	    {
	      return -1;
	    }
	  if (p->type == PART_TYPE_EXACT)
	    {
	      r += strlen (p->match);
	    }
	  else
	    {
	      r++;
	    }
	  p->hits = 0;
	  p = p->next;
	  break;
	default:
	  LOG_ERR ("Unknown type %d", p->type);
	  return -1;
	}

    }
  DINFO (10, "%p %d, %d", p, r - text, *ind);
  len = r - text - *ind;
  ret = 0;
  sm->current = NULL;
  scanner->current_match = NULL;
  if (hit == 0)
    {
      return -1;
    }
  if (p == NULL)
    {
      goto out;
    }
  DINFO (10, "No more to parse while parsing %d", sm->id);

  if (sm->multiline && hit > 0)
    {
      scanner->current_match = sm;
      sm->current = p;
      ret = -2;
      //len = -1;
      goto out;
    }
  while (p)
    {
      switch (p->mult)
	{
	case '*':
	  /* Ok. */
	  break;
	case '+':
	  if (p->hits == 0)
	    {
	      return -1;
	    }
	  break;
	case '?':
	  if (p->hits == 0)
	    {
	      return -1;
	    }
	  break;
	default:
	  return -1;
	}
      p->hits = 0;
      p = p->next;
    }

out:
  /* Match */
  DINFO (7, "Match found for '%.*s'", r - text - *ind, text);
  if (id_table_add (idt, sm->id, *ind, len) < 0)
    {
      return -1;
    }
  *ind = r - text;

  return ret;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a scanner.
 *
 * Creates a scanner.
 *
 * @return The new scanner. NULL if failed to create the scanner.
 */
vsscanner * vsscanner_create ()
{
  vsscanner *scanner;

  scanner = (vsscanner *) malloc (sizeof (*scanner));
  memset (scanner, 0, sizeof (*scanner));

  scanner->state = STATE_NONE;

  DINFO (1, "Scanner created");

  return scanner;
}

/**
 * @brief Free a scanner.
 *
 * Free a scanner and it's resources.
 *
 * @param scanner The scanner to be set free.
 */
void
vsscanner_free (vsscanner * scanner)
{
  int i;
  part_match *pm;
  part_match *next;

  assert (scanner);

  for (i = 0; i < scanner->normal.len; i++)
    {
      pm = scanner->normal.matches[i].first;
      while (pm)
	{
	  next = pm->next;
	  if (pm->match != NULL)
	    {
	      free (pm->match);
	    }
	  free (pm);
	  pm = next;
	}
    }
  if (scanner->normal.matches)
    {
      free (scanner->normal.matches);
    }

  for (i = 0; i < scanner->start.len; i++)
    {
      pm = scanner->start.matches[i].first;
      while (pm)
	{
	  next = pm->next;
	  if (pm->match != NULL)
	    {
	      free (pm->match);
	    }
	  free (pm);
	  pm = next;
	}
    }
  if (scanner->start.matches)
    {
      free (scanner->start.matches);
    }
  free (scanner);
  DINFO (1, "Scanner freed");
}

/**
 * @brief Restart the scanner.
 *
 * Restart the scanner. Should be called before starting to scan, or if a new
 * file is being scanned.
 *
 * @param scanner The scanner.
 */
void
vsscanner_restart (vsscanner * scanner)
{
  assert (scanner);

  scanner->state = STATE_NONE;
  scanner->current_match = NULL;
}

/**
 * @brief Scan a line of text.
 *
 * @param scanner The scanner.
 * @param text The line to be scanned.
 * @param ids The result table.
 *
 * @return 0 upon success.
 */
int
vsscanner_scan (vsscanner * scanner, const char *text, id_table * ids)
{
  const char *p = text;
  int ret = 0;
  int i;
  int imax;
  int ind = 0;
  scan_match *smp;

  assert (scanner);
  assert (ids);

  ind = 0;
  DINFO (10, "Scanning State %d '%s'", scanner->state, text);
  if (scanner->state == STATE_MATCH && scanner->current_match)
    {
      ids->id[0].id = scanner->current_match->id;
      ids->id[0].index = 0;
      ids->id[0].len = 0;
      ids->len = 1;
      if (text == NULL || strlen (text) == 0)
	{
	  return 0;
	}
      ret = vsscanner_scan_match (scanner, scanner->current_match, text,
				  &ind, ids);
      if (ret == -2)
	{
	  /* No more text */
	  return 0;
	}
    }
  else
    {
      scanner->state = STATE_START;
      if (text == NULL || strlen (text) == 0)
	{
	  return 0;
	}
    }

  while (*p)
    {
      DINFO (10, "Scanning State %d '%s' '%s' (%d)", scanner->state, p,
	     &text[ind], ind);
      if (scanner->state == STATE_START && scanner->start.len > 0)
	{
	  smp = scanner->start.matches;
	  imax = scanner->start.len;
	}
      else
	{
	  smp = scanner->normal.matches;
	  imax = scanner->normal.len;
	}

      for (i = 0; i < imax; i++)
	{
	  ret = vsscanner_scan_match (scanner, &smp[i], text, &ind, ids);
	  if (ret == 0)
	    {
	      /* Match ok, start over. */
	      p = &text[ind];
	      break;
	    }
	  else if (ret == -2)
	    {
	      /* No more text. */
	      return 0;
	    }
	  /* did not match */
	}
      scanner->state = STATE_MATCH;
      if (ret < 0)
	{
	  if (smp != scanner->start.matches)
	    {
	      p++;
	      ind++;
	    }
	}
    }
  return 0;
}

/**
 * @brief Add a new expression rule.
 *
 * Add a new expression for matching.
 *
 * @param scanner The scanner.
 * @param rule The rule.
 * @param id The id corrresponding to the rule.
 * @param multiline Set to 1 if the match could be several lines, e.g. comments.
 * @param word Set to 1 if the match is a word.
 *
 * @return 0 upon success.
 */
int
vsscanner_add_rule (vsscanner * scanner, const char *rule, int id,
		    int multiline, int word)
{
  scan_match *sm;
  scan_match *new_sm;
  scan_group *sg;
  int i;
  const char *start;

  assert (scanner);

  DINFO (3, "Adding id %d rule '%s'", id, rule);

  if (*rule == '^')
    {
      sg = &scanner->start;
      start = rule + 1;
    }
  else
    {
      sg = &scanner->normal;
      start = rule;
    }

  if (sg->len == sg->size)
    {
      sg->size += SCAN_INCREASE;
      new_sm = (scan_match *) realloc (sg->matches, sg->size * sizeof (*sm));
      if (new_sm == NULL)
	{
	  return -1;
	}
      sg->matches = new_sm;
      for (i = sg->len; i < sg->size; i++)
	{
	  new_sm[i].current = NULL;
	  new_sm[i].first = NULL;
	}
    }
  sm = &sg->matches[sg->len];
  sm->id = id;
  sm->multiline = multiline;
  sm->word = word;
  if (word)
    {
      /* Create the word directly. */
      sm->first = (part_match *) malloc (sizeof (part_match));
      memset (sm->first, 0, sizeof (part_match));
      if (sm->first == NULL)
	{
	  return -1;
	}
      sm->first->match = strdup (start);
      if (sm->first->match == NULL)
	{
	  free (sm->first);
	  return -1;
	}
      DINFO (10, "Added word rule '%s'", start);
    }
  else
    {
      sm->first = part_match_create (start);
    }
  if (sm->first != NULL)
    {
      sg->len++;
      return 0;
    }

  return -1;
}
