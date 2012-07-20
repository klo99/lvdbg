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

/**
 * @file configuration.c
 *
 * @brief Implements the configuration parser.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "configuration.h"
#include "debug.h"

#define ESCAPE_CHAR '\\'
#define COMMENT_CHR '#'
#define EQUAL_CHR   "="
#define GROUP_START '['
#define GROUP_END   "]"
#define EMPTY_STRING ""

#define C_ESCAPES_CHARS "ntvbrfa\\?'\""
#define C_ESCAPES "\n\t\v\b\r\f\a\\\?\'\""

/** Number of bytes for a configuration line. */
#define LINE_LEN 4096

#define NOT_VALID "Not a valid %s value at line %d"
#define NO_PARAM "No parameter '%s' in group '%s'"

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
typedef struct conf_entry_t conf_entry;
typedef struct conf_group_t conf_group;

/** Structure for a parameter entry in the configuration. */
struct conf_entry_t
{
  conf_parameter parameter; /**< The parameter. */
  conf_entry *next;	    /**< Next parameter in the list. */
};

/** Structure to hold information about a configuration sub group. */
struct conf_group_t
{
  char *group_name;    /**< The group name. For the root group it is NULL. */
  conf_entry *entries; /**< List of parameter entries. */
  conf_group *next;    /**< Next sub group of the configuration. */
};

/** Structure holding all the configuration parameters. */
struct configuration_t
{
  conf_group *groups; /**< A list of all sub groups. */
  conf_group *latest; /**< Latest group that was used. */
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static int conf_check_line (char *line);
static int conf_c_to_string (char *text);
static char *conf_get_token (char *line, char **start_token, char *delim,
			     int null_ok);
static conf_group *conf_get_group (configuration * conf, const char *name);
static conf_entry *conf_get_entry (conf_group * group, const char *name);

/**
 * @brief Check line from configuation file.
 *
 * Check the last signigicant, non space and non comment, and end line
 * after that. If there only were spaces/comments we can skip this line.
 * Check if line ends with '\\', and if so we should read another line.
 *
 * @param line The line to check.
 *
 * @return -2 if only spaces and comments. -1 if line ends with '\\'. 0 if
 *         line has significant text, parameter of subgroup.
 */
static int
conf_check_line (char *line)
{
  char *r;
  char *last_non_space = NULL;
  int ret = 0;
  int in_quote = 0;
  int in_dquote = 0;

  if (line == NULL || strlen (line) < 2)
    {
      return -2;
    }
  /* Read next line if this line ends with '\\' */
  if (line[strlen (line) - 2] == '\\')
    {
      ret = -1;
      line[strlen (line) - 2] = '\0';
    }

  /* Scan for Comment's */
  r = line;
  while (*r && (*r != '\\' || *(r + 1) != '\0'))
    {
      if (!isblank (*r) && *r != COMMENT_CHR)
	{
	  last_non_space = r;
	}

      if (in_quote)
	{
	  if (*r == '\'' && *(r - 1) != '\\')
	    {
	      in_quote = 0;
	    }
	}
      else if (in_dquote)
	{
	  if (*r == '"' && *(r - 1) != '\\')
	    {
	      in_dquote = 0;
	    }
	}
      else if (*r == '\'')
	{
	  in_quote = 1;
	}
      else if (*r == '"')
	{
	  in_dquote = 1;
	}
      else if (*r == COMMENT_CHR)
	{
	  /* No need to continue. */
	  break;
	}
      r++;
    }

  if (last_non_space != NULL)
    {
      *(last_non_space + 1) = '\0';
    }
  else if (*r == COMMENT_CHR)
    {
      /* Only comments on line */
      ret = -2;
    }
  DINFO (5, "Ret %d %d - '%s'", ret,
	 last_non_space ? last_non_space - line : -1, line);
  return ret;
}

/**
 * @brief A converter from a C-string to a normal string.
 *
 * Converts a C-string to a normal string. "\n\t\v\b\r\f\a\\\?\'\"" are
 * unescaped to their real values. '\\xHH' and '\\ooo' are also converted to
 * the corresponding byte values.
 *
 * @param text The text that should be converted. Should be null terminated.
 *
 * @return 0 if successful, otherwise -1.
 */
static int
conf_c_to_string (char *text)
{
  char *r;
  char *w;
  char *p;
  char nr[4];

  r = text;
  w = text;
  while (*r)
    {
      if (*r == ESCAPE_CHAR)
	{
	  if ((p = strchr (C_ESCAPES_CHARS, *(r + 1))) != NULL)
	    {
	      *w = C_ESCAPES[p - C_ESCAPES_CHARS];
	      /* Convert the escape character. */
	      w++;
	      r += 2;
	    }
	  else if (*(r + 1) == 'x')
	    {
	      /* Convert from hexadecimal. */
	      if (isxdigit (r[2]) && isxdigit (r[3]))
		{
		  nr[0] = r[2];
		  nr[1] = r[3];
		  nr[2] = '\0';
		  *w = (char) strtol (nr, NULL, 16);
		  w++;
		  r += 4;
		}
	      else
		{
		  goto error;
		}
	    }
	  else if (r[1] >= '0' && r[1] <= '7')
	    {
	      /* Convert the octal to decimal. */
	      nr[0] = r[1];
	      if (r[2] >= '0' && r[2] <= '7')
		{
		  nr[1] = r[2];
		  if (r[3] >= '0' && r[3] <= '7')
		    {
		      nr[2] = r[3];
		      nr[3] = '\0';
		      r += 4;
		    }
		  else
		    {
		      nr[2] = '\0';
		      r += 3;
		    }
		}
	      else
		{
		  r += 2;
		  nr[1] = '\0';
		}
	      *w = strtol (nr, NULL, 8);
	      w++;
	    }
	  else
	    {
	      /* Not a valid escape sequence. */
	      goto error;
	    }
	}
      else
	{
	  /* 'Normal' characters are copied. */
	  *w = *r;
	  w++;
	  r++;
	}
    }
  /* Set the string termination. */
  *w = '\0';

  return 0;

error:
  return -1;
}

/**
 * @brief Retrieves the token seperated by the delim characters.
 *
 * Retrieves a token from a string by using the delimiters specified. The nil
 * '\\0' could also be valid if null_ok is set. Spaces before and after the
 * token are stripped.
 *
 * The input text is altered. A '\\0' is inserted at the end of the token.
 *
 * @param line Text where the token is parsed from.
 * @param start_token Points to start the of the token.
 * @param delim Delimiters used for specify the token.
 * @param null_ok If set to 1 a null byte '\\0' is also considered as a valid
 *                deliminater.
 *
 * @return If a token was found we return a pointer to the character after
 *         the deliminater. If no token was found NULL is returned.
 *
 */
static char *
conf_get_token (char *line, char **start_token, char *delim, int null_ok)
{
  char *p = line;
  char *end_token;

  /* Skip spaces. */
  while (*p && isspace (*p))
    {
      p++;
    }
  if (*p == '\0')
    {
      if (null_ok)
	{
	  /* Ok if caller says so. */
	  *start_token = p;
	  return p;
	}
      return NULL;
    }

  /* Find out where toke ends, the last non space before delim. */
  *start_token = p;
  end_token = p;
  while (*p && strchr (delim, *p) == NULL)
    {
      if (!isspace (*p))
	{
	  end_token = p;
	}
      p++;
    }
  if (*p == '\0' && !null_ok)
    {
      return NULL;
    }

  /*
   * Set nil to specify the token and return pointer to the char after
   * delim.
   */
  p = *p == '\0' ? p : (p + 1);
  *(end_token + 1) = '\0';
  return p;
}


/**
 * @brief Get a sub group.
 *
 * Retrieves a sub group with the specified name.
 *
 * @param conf Configuration structure were the sub group is located.
 * @param name Name of the sub group.
 *
 * @return Pointer to the sub group if it exists. NULL if the sub group does
 *         not exist.
 */
static conf_group *
conf_get_group (configuration * conf, const char *name)
{
  conf_group *group = conf->groups;
  int c = -1;

  if (name == NULL)
    {
      if (group && group->group_name == NULL)
	{
	  return group;
	}
      /* No root group is defined. */
      return NULL;
    }

  /* Start looking at the first non root group. */
  if (group && group->group_name == NULL)
    {
      group = group->next;
    }

  /*
   * The groups are inserted in order so only necessary to look until
   * the names are larger then the specified.
   */
  while (group && (c = strcmp (group->group_name, name)) < 0)
    {
      group = group->next;
    }
  return c == 0 ? group : NULL;
}

/**
 * @brief Get a parameter entry.
 *
 * Retrieves a parameter entry from a specified group.
 *
 * @param group The group were the parameter is.
 * @param name The name of the parameter.
 *
 * @return Pointer to the paramter entry if we find it. Otherwise NULL.
 */
static conf_entry *
conf_get_entry (conf_group * group, const char *name)
{
  conf_entry *entry = group->entries;
  int c = -1;

  /* The entries are inserted in order. */
  while (entry && (c = strcmp (entry->parameter.name, name)) < 0)
    {
      entry = entry->next;
    }
  return c == 0 ? entry : NULL;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a configuration structure.
 *
 * Creates an empty configuration object.
 *
 * @return A pointer to the new configuration.
 */
configuration *
conf_create ()
{
  configuration *conf;

  conf = (configuration *) malloc (sizeof (*conf));
  LOG_ERR_IF_FATAL (conf == NULL, ERR_MSG_CREATE ("configuration"));
  conf->groups = NULL;
  conf->latest = NULL;

  return conf;
}

/**
 * @brief Releases the resources for the configuration.
 *
 * Release a configuration's resources and the object itself.
 *
 * @param conf configuration that should be set free.
 */
void
conf_free (configuration * conf)
{
  conf_group *group;
  conf_group *next;
  conf_entry *entry;
  conf_entry *enext;

  /* Delete all the sub groups. */
  group = conf->groups;
  while (group)
    {
      entry = group->entries;
      /* Delte all entries in the sub group. */
      while (entry)
	{
	  if (entry->parameter.type == PARAM_STRING)
	    {
	      if (entry->parameter.value.string_value)
		{
		  free (entry->parameter.value.string_value);
		}
	      if (entry->parameter.default_value.string_value)
		{
		  free (entry->parameter.default_value.string_value);
		}
	    }
	  if (entry->parameter.name)
	    {
	      free (entry->parameter.name);
	    }
	  enext = entry->next;
	  free (entry);
	  entry = enext;
	}
      if (group->group_name)
	{
	  free (group->group_name);
	}
      next = group->next;
      free (group);
      group = next;
    }
  free (conf);
}

/**
 * @brief Add a new sub group to the configuration.
 *
 * Adds a new sub group to the configuration. If no name is given, it is
 * the root group. Only one root group could be added.
 *
 * @todo Not allowing two sub groups with the same name. As it is now
 * a second group with the same name hides the previous group.
 *
 * @param conf The configuration that the group is added to.
 * @param name Name of the sub group, or NULL it it is the root group.
 * @param parameters A list of parameters that are in the group. The last
 *                   element must have the parameter name = NULL.
 *
 * @return 0 if all parameters were added to the group. -1 if not all
 *         parameters were set. All preceding parameters and the group
 *         are added to the configuration.
 */
int
conf_add_group (configuration * conf, const char *name,
		const conf_parameter * parameters)
{
  conf_group *new_group;
  conf_group *p;
  conf_group *pprev;
  conf_entry *new_entry;
  conf_entry *pe;
  conf_entry *peprev;
  const conf_parameter *param;

  /* We can only add if has created the configuration. */
  LOG_ERR_IF_RETURN (conf == 0, -1, "Configuration not created");

  /* Create the sub group. */
  if (name == NULL)
    {
      /* No name means that these parameters belong to the root. */
      if (conf->groups && conf->groups->group_name == NULL)
	{
	  LOG_ERR ("Already created a root group");
	  return -1;
	}
      new_group = (conf_group *) malloc (sizeof (*new_group));
      new_group->next = conf->groups;
      new_group->entries = NULL;
      new_group->group_name = NULL;
      conf->groups = new_group;
    }
  else
    {
      new_group = (conf_group *) malloc (sizeof (*new_group));
      new_group->group_name = strdup (name);
      new_group->entries = NULL;
      pprev = NULL;
      p = conf->groups;
      while (p != NULL && (p->group_name == NULL
			   || strcmp (p->group_name, name) < 0))
	{
	  pprev = p;
	  p = p->next;
	}
      DINFO (1, "Created sub group %s", name == NULL ? "" : name);
      if (pprev == NULL)
	{
	  new_group->next = conf->groups;
	  conf->groups = new_group;
	}
      else
	{
	  pprev->next = new_group;
	  new_group->next = p;
	}
    }

  /* Add parameters to group. */
  param = parameters;
  while (param != NULL && param->name != NULL)
    {
      if (param->type < 0 || param->type > PARAM_LAST)
	{
	  /*
	   * We already inserted the group. So keep the group and the
	   * parameters we already got, but return error.
	   */
	  return -1;
	}
      new_entry = (conf_entry *) malloc (sizeof (*new_entry));
      memcpy (&new_entry->parameter, param, sizeof (*param));
      new_entry->parameter.name = strdup (param->name);
      if (param->type == PARAM_STRING)
	{
	  new_entry->parameter.value.string_value =
	    strdup (param->default_value.string_value);
	  new_entry->parameter.default_value.string_value =
	    strdup (param->default_value.string_value);
	}
      else
	{
	  new_entry->parameter.value = param->default_value;
	}
      pe = new_group->entries;
      peprev = NULL;
      while (pe && strcmp (pe->parameter.name, param->name) < 0)
	{
	  peprev = pe;
	  pe = pe->next;
	}
      DINFO (1, "Created parameter %s of type %d", new_entry->parameter.name,
	     new_entry->parameter.type);
      if (peprev == 0)
	{
	  new_entry->next = new_group->entries;
	  new_group->entries = new_entry;
	}
      else
	{
	  new_entry->next = pe;
	  peprev->next = new_entry;
	}
      param++;
    }
  return 0;
}

/**
 * @brief Load a configuration from a file.
 *
 * Loads a configuration file.
 *
 * @param conf The configuration we should load to.
 * @param file_name File name of the configuration file.
 *
 * @return 0 if the file were loaded ok. If an error occured -linenumber is
 * return.
 */
int
conf_load (configuration * conf, const char *file_name)
{
  conf_group *group = conf->groups;
  conf_entry *entry;
  int line_nr = 0;
  FILE *file;
  char line[LINE_LEN];
  char *p;
  char *q;
  int first_group = 1;
  int i;
  float f;
  int read_len;
  int ret;

  DINFO (1, "Reading conf from '%s'", file_name);

  /* Sanity checks */
  LOG_ERR_IF_RETURN (group == NULL, -1, "No groups are added");

  LOG_ERR_IF_RETURN (file_name == NULL, -1, "No file name");

  file = fopen (file_name, "r");
  if (file == NULL)
    {
      LOG_ERR ("Could not open '%s': %m", file_name);
      /* Just return. */
      return -1;
    }

  read_len = 0;
  /* Parse one row at the time. */
  while (!feof (file) || read_len > 0)
    {
      p = fgets (line + read_len, LINE_LEN - read_len, file);
      if (p == NULL && read_len == 0)
	{
	  break;
	}

      line_nr++;
      ret = conf_check_line (line);
      if (ret == -1)
	{
	  read_len = strlen (line);
	  DINFO (3, "Continue after %d", read_len);
	  if (!feof (file) && read_len < LINE_LEN)
	    {
	      continue;
	    }
	  /*
	   * Parse the last line even if it ended with '\n' or if we
	   * filled the buffer to LINE_LEN.
	   */
	}
      else if (ret == -2)
	{
	  read_len = 0;
	  continue;
	}

      /* We will parse the whole line so set read_len to 0. */
      read_len = 0;

      p = line;
      while (*p && isspace (*p))
	{
	  p++;
	}
      if (*p == GROUP_START)
	{
	  /* New sub group. */
	  p = conf_get_token (p + 1, &q, "]", 0);
	  if (p == NULL || strlen (p) == 0)
	    {
	      LOG_ERR ("Could not find group name at line %d", line_nr);
	      goto error;
	    }
	  group = conf_get_group (conf, q);
	  if (group == NULL)
	    {
	      LOG_ERR ("Could not find group '%s'", q);
	      goto error;
	    }
	  first_group = 0;
	  continue;
	}

      /* Parameters section. */
      if (first_group && group->group_name != NULL)
	{
	  /*
	   * We do not have a root group, but we got parameters to the root
	   * group.
	   */
	  LOG_ERR ("Found root parameters at line %d, but we do not have any",
		   line_nr);
	  goto error;
	}
      /* Get Parameter name and it's entry. */
      p = conf_get_token (p, &q, "=", 0);
      if (p == NULL)
	{
	  LOG_ERR ("Could not find '=' at line %d", line_nr);
	  goto error;
	}
      entry = conf_get_entry (group, q);
      if (entry == NULL)
	{
	  LOG_ERR ("Could not find parameter '%s'", q);
	  goto error;
	}

      /* Get the parameter value. */
      p = conf_get_token (p, &q, "\r\n", 1);
      /* p is never NULL, no need to check. */

      /* Skip " and ' in e.g. symbol = 'B' */
      if ((*q == '\"' || *q == '\'') && *q == q[strlen (q) - 1])
	{
	  if (*q == '\"')
	    {
	      if (conf_c_to_string (q) < 0)
		{
		  LOG_ERR (NOT_VALID, "c-string", line_nr);
		  goto error;
		}
	    }
	  q[strlen (q) - 1] = '\0';
	  q++;
	}

      /* Update the value of the parameter. */
      switch (entry->parameter.type)
	{
	case PARAM_STRING:
	  if (entry->parameter.value.string_value == NULL
	      || strlen (entry->parameter.value.string_value) <
	      strlen (q) + 1)
	    {
	      /* We must create a new string. */
	      if (entry->parameter.value.string_value)
		{
		  free (entry->parameter.value.string_value);
		}
	      entry->parameter.value.string_value = strdup (q);
	      LOG_ERR_IF_FATAL (entry->parameter.value.string_value == NULL,
				ERR_MSG_CREATE ("parameter string"));
	    }
	  else
	    {
	      sprintf (entry->parameter.value.string_value, "%s", q);
	    }
	  break;
	case PARAM_BOOL:
	  if (strcasecmp (q, "yes") == 0 || strcasecmp (q, "enable") == 0
	      || strcasecmp (q, "true") == 0 || strcmp (q, "1") == 0)
	    {
	      entry->parameter.value.bool_value = 1;
	    }
	  else if (strcasecmp (q, "no") == 0 || strcasecmp (q, "disable") == 0
		   || strcasecmp (q, "false") == 0 || strcmp (q, "0") == 0)
	    {
	      entry->parameter.value.bool_value = 0;
	    }
	  else
	    {
	      LOG_ERR (NOT_VALID, "bool", line_nr);
	      goto error;
	    }
	  break;
	case PARAM_INT:
	  i = strtol (q, &p, 0);
	  if (p == q || *p != '\0')
	    {
	      LOG_ERR (NOT_VALID, "int", line_nr);
	      goto error;
	    }
	  if (i < entry->parameter.min)
	    {
	      i = entry->parameter.min;
	    }
	  else if (i > entry->parameter.max)
	    {
	      i = entry->parameter.max;
	    }
	  entry->parameter.value.int_value = i;
	  break;
	case PARAM_UINT:
	  i = strtoul (q, &p, 0);
	  if (p == q || *p != '\0')
	    {
	      LOG_ERR (NOT_VALID, "uint", line_nr);
	      goto error;
	    }
	  if (i < entry->parameter.min)
	    {
	      i = entry->parameter.min;
	    }
	  else if (i > entry->parameter.max)
	    {
	      i = entry->parameter.max;
	    }
	  entry->parameter.value.uint_value = i < 0 ? 0 : i;
	  break;
	case PARAM_CHAR:
	  if (strlen (q) > 1)
	    {
	      LOG_ERR (NOT_VALID, "char", line_nr);
	      goto error;
	    }
	  entry->parameter.value.char_value = *q;
	  break;
	case PARAM_FLOAT:
	  f = strtof (q, &p);
	  if (p == q || *p != '\0')
	    {
	      LOG_ERR (NOT_VALID, "float", line_nr);
	      goto error;
	    }
	  if (f < entry->parameter.min)
	    {
	      f = entry->parameter.min;
	    }
	  else if (f > entry->parameter.max)
	    {
	      f = entry->parameter.max;
	    }
	  entry->parameter.value.float_value = f;
	  break;
	default:
	  /* Should not happen, as type is checked when adding a group... */
	  LOG_ERR ("This can _not_ happen... %d", line_nr);
	  goto error;
	}
    }
  fclose (file);
  return 0;

error:
  LOG_ERR ("Bailing out.");
  fclose (file);
  return line_nr == 0 ? -1 : -line_nr;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
unsigned int
conf_get_uint (configuration * conf, const char *group_name, const char *name,
	       int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return 0;
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.uint_value;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
int
conf_get_bool (configuration * conf, const char *group_name, const char *name,
	       int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return 0;
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.bool_value;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter. The caller
 * should not free the string.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
const char *
conf_get_string (configuration * conf, const char *group_name,
		 const char *name, int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return EMPTY_STRING;
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.string_value;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
int
conf_get_int (configuration * conf, const char *group_name, const char *name,
	      int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return 0;
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.int_value;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
char
conf_get_char (configuration * conf, const char *group_name, const char *name,
	       int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return '?';
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.char_value;
}

/**
 * @brief Retrieve parameter value.
 *
 * Retrieves the parameter value for the specified parameter.
 *
 * @param conf configuration structure.
 * @param group_name The sub group name, or NULL for root group.
 * @param name Name of the parameter.
 * @param valid Is set to 1 if the value is ok, i.e. the group and the and
 *        parameter exists. If they do not exist -1 is returned.
 *
 * @return Value of the parameter.
 */
float
conf_get_float (configuration * conf, const char *group_name,
		const char *name, int *valid)
{
  conf_group *group;
  conf_entry *entry;

  group = conf_get_group (conf, group_name);
  if (group == NULL || (entry = conf_get_entry (group, name)) == NULL)
    {
      if (valid)
	{
	  *valid = 0;
	}
      LOG_ERR (NO_PARAM, name, group_name);
      return 0.0f;
    }
  if (valid)
    {
      *valid = 1;
    }
  return entry->parameter.value.float_value;
}
