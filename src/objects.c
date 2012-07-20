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
 * @file objects.c
 *
 * @brief Implements common debug obejcts.
 *
 * @li breakpoint holds information about a breakpoint.
 * @li breakpoint_table holds information about a set of breakpoints.
 * @li thread holds the information about a thread.
 * @li thread_group Hold information of a group.
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "objects.h"
#include "debug.h"
#include "lvdbg.h"
#include "misc.h"

#define BP_START 10
#define BP_INCREASE 10

#define FRAME_INCREASE 10

/*******************************************************************************
 * Public functions
 ******************************************************************************/

/*******************************************************************************
 * breakpoint functions
 ******************************************************************************/

/**
 * @name Breakpoint objects
 *
 * Breakpoint helper objects. Objects used by the parser to handle breakpoint
 * information.
 */
/*@{*/
/**
 * @brief Creates a breakpoint table.
 *
 * Creates a breakpoint table. All breakpoints are set to NULL.
 *
 * @return A pointer to the new breakpoint table or NULL if it fails to
 *         create the table.
 */
breakpoint_table *
bp_table_create (void)
{
  breakpoint_table *bpt;

  bpt = (breakpoint_table *) malloc (sizeof (*bpt));
  LOG_ERR_IF_FATAL (bpt == NULL, ERR_MSG_CREATE ("breakpoint table"));

  bpt->breakpoints =
    (breakpoint **) malloc (BP_START * sizeof (breakpoint *));
  LOG_ERR_IF_FATAL (bpt == NULL, ERR_MSG_CREATE ("breakpoints"));

  memset (bpt->breakpoints, 0, BP_START * sizeof (breakpoint *));
  bpt->rows = BP_START;
  bpt->cols = 0;
  bpt->in_use = 0;

  DINFO (1, "breakpoint table created.");
  return bpt;
}

/**
 * @brief Free a breakpoint table.
 *
 * Free a breakpoint table and the breakpoints in the table.
 *
 * @param bpt The breakpoint table to be set free.
 */
void
bp_table_free (breakpoint_table * bpt)
{
  int i;

  assert (bpt);

  for (i = 0; i < bpt->rows; i++)
    {
      if (bpt->breakpoints[i] != NULL)
	{
	  bp_free (bpt->breakpoints[i]);
	}
    }
  free (bpt->breakpoints);
  free (bpt);
}

/**
 * @brief Insert a breakpoint in the table.
 *
 * Insert a breakpoint in the table. If a breakpoint with the same number
 * already exists, the old breakpoint is freed.
 *
 * @param bpt The breakpoint table that inserts the breakpoint.
 * @param bp The breakpoint to be inserted.
 *
 * @return 0 if the breakpoint was inserted, otherwise -1.
 */
int
bp_table_insert (breakpoint_table * bpt, breakpoint * bp)
{
  int rows;
  int i;

  assert (bpt);
  assert (bp);

  if (bp->number < 0)
    {
      LOG_ERR ("Bad breakpoint number %d", bp->number);
      return -1;
    }

  rows = bpt->rows;
  if (rows <= bp->number)
    {
      bpt->breakpoints = (breakpoint **) realloc (bpt->breakpoints,
						  (bp->number +
						   BP_INCREASE) *
						  sizeof (*bp));
      LOG_ERR_IF_FATAL (bpt->breakpoints == NULL,
			ERR_MSG_CREATE ("breakpoints"));
      for (i = rows; i < bp->number + BP_INCREASE; i++)
	{
	  bpt->breakpoints[i] = NULL;
	}
      bpt->rows = bp->number + BP_INCREASE;
    }
  if (bpt->breakpoints[bp->number] != NULL)
    {
      bp_free (bpt->breakpoints[bp->number]);
    }
  else
    {
      bpt->in_use++;
    }
  bpt->breakpoints[bp->number] = bp;
  DINFO (1, "Inserted breakpoint number %d", bp->number);

  return 0;
}

/**
 * @brief Remove breakpoint from table.
 *
 * Remove a breakpoint from a breakpoint table.
 *
 * @param bpt The breakpoint table.
 * @param number The breakpoint number to be removed.
 *
 * @return 0 If the breakpoint was removed. -1 if it does not exists.
 */
int
bp_table_remove (breakpoint_table * bpt, int number)
{
  assert (bpt);

  LOG_ERR_IF_RETURN (number < 0 || number >= bpt->rows
		     || bpt->breakpoints[number] == NULL, -1,
		     "Not a valid breakpoint %d", number);
  bp_free (bpt->breakpoints[number]);
  bpt->in_use--;
  bpt->breakpoints[number] = NULL;

  DINFO (5, "Breakpoint nr %d removed", number);

  return 0;
}

/**
 * @brief Create a breakpoints.
 *
 * Creates a breakpoint. All fields are set to NULL or 0.
 *
 * @return The pointer to the new breakpoint or NULL if it failed to create
 *         one.
 */
breakpoint *
bp_create (void)
{
  breakpoint *bp;

  bp = (breakpoint *) malloc (sizeof (*bp));
  LOG_ERR_IF_FATAL (bp == 0, ERR_MSG_CREATE ("breakpoint"));

  memset (bp, 0, sizeof (*bp));
  bp->thread = -1;

  return bp;
}

/**
 * @brief Free a breakpoint.
 *
 * Free a breakpoint and it's resources.
 *
 * @param bp The breakpoint to be set free.
 */
void
bp_free (breakpoint * bp)
{
  assert (bp);

  DINFO (1, "Freeing breakpoint %d", bp->number);
  if (bp->func != NULL)
    {
      free (bp->func);
    }
  if (bp->file != NULL)
    {
      free (bp->file);
    }
  if (bp->fullname != NULL)
    {
      free (bp->fullname);
    }
  if (bp->cond != NULL)
    {
      free (bp->cond);
    }
  if (bp->original_location != NULL)
    {
      free (bp->original_location);
    }
  if (bp->expression != NULL)
    {
      free (bp->expression);
    }
  if (bp->value != NULL)
    {
      free (bp->value);
    }

  free (bp);
}

/*@}*/

/*******************************************************************************
 * thread functions
 ******************************************************************************/

/**
 * @name Thread objects
 *
 * Thread helper functions. Functions for handle the thread information of the
 * running processes.
 */
/*@{*/
/**
 * @brief Add a new thread group.
 *
 * Adds a new thread group to head.
 *
 * @param head The first thread group, or NULL if no groups exists.
 * @param group_id The id for the group.
 */
int
thread_group_add (thread_group ** head, int group_id)
{
  thread_group *p;

  assert (head);

  /* Sanity check that we do not have the group already. */
  p = *head;
  while (p != NULL)
    {
      LOG_ERR_IF_RETURN (p->id == group_id, -1, "Group already added: %d",
			 group_id);
      p = p->next;
    }

  p = (thread_group *) malloc (sizeof (*p));
  LOG_ERR_IF_FATAL (p == NULL, "Failed to create thread group");

  p->id = group_id;
  p->first = NULL;
  p->next = *head;
  *head = p;

  DINFO (3, "Created group %d", group_id);

  return 0;
}

/**
 * @brief Get thread.
 *
 * Get a thread with id @a thread_id in group with id @a group_id.
 *
 * @param head The list of groups.
 * @param group_id The group id. If -1 all groups a searched.
 * @param thread_id The thread id.
 *
 * @return A pointer with the specified id. If the thread does not exists
 *         NULL is returned.
 */
thread *
thread_group_get_thread (thread_group * head, int group_id, int thread_id)
{
  thread_group *pg;
  thread *pt;

  pg = head;
  while (pg)
    {
      if (pg->id == group_id || group_id == -1)
	{
	  pt = pg->first;
	  while (pt)
	    {
	      if (pt->id == thread_id)
		{
		  return pt;
		}
	      pt = pt->next;
	    }
	}
      pg = pg->next;
    }
  return NULL;
}

/**
 * @brief Remove a group.
 *
 * Remove group with id \a group_id. The group and all it's threads are freed.
 *
 * @param head The firest group to look at.
 * @param group_id The id to remove.
 *
 * @return 0 if the group was removed. -1 if no group with id @a group_id.
 */
int
thread_group_remove (thread_group ** head, int group_id)
{
  thread_group *p;
  thread_group *prev = NULL;

  assert (head);

  p = *head;
  while (p != NULL && p->id != group_id)
    {
      prev = p;
      p = p->next;
    }
  LOG_ERR_IF_RETURN (p == NULL, -1, "No group found for %d", group_id);

  if (prev == NULL)
    {
      *head = p->next;
    }
  else
    {
      prev->next = p->next;
    }

  thread_remove_all (&p->first);
  free (p);

  DINFO (3, "Group %d removed", group_id);

  return 0;
}

/**
 * @brief Removes all group.
 *
 * Remove all groups and their threads. All groups and threads are freed.
 *
 * @param head Firts group to remove.
 */
void
thread_group_remove_all (thread_group ** head)
{
  thread_group *p;
  thread_group *next;

  assert (head);

  p = *head;
  while (p != NULL)
    {
      next = p->next;
      thread_remove_all (&p->first);
      free (p);
      p = next;
    }
  *head = NULL;

  DINFO (3, "All groups removed");
}

/**
 * @brief Add a thread.
 *
 * Add a thread with \a thread_id to the group with \a group_id.
 *
 * @param head First group.
 * @param group_id Id of the group that the thread should be added to.
 * @param thread_id Id of the group.
 *
 * @return
 */
int
thread_add (thread_group * head, int group_id, int thread_id)
{
  thread_group *p;
  thread *t;

  p = head;
  while (p != NULL && p->id != group_id)
    {
      p = p->next;
    }
  LOG_ERR_IF_RETURN (p == NULL, -1, "Could not find group %d", group_id);

  t = (thread *) malloc (sizeof (*t));
  LOG_ERR_IF_FATAL (t == NULL, "Could not create thread");
  memset (&t->frame, 0, sizeof (frame));

  t->id = thread_id;
  t->running = 0;
  t->next = p->first;
  p->first = t;

  DINFO (1, "Created thread %d in group %d", thread_id, group_id);

  return 0;
}

/**
 * @brief Remove a thread.
 *
 * Remove thread with id \a thread_id belonging to group \a group_id. The
 * thread is freed.
 *
 * @param head The first group.
 * @param group_id The group id of the thread.
 * @param thread_id The id of the thread that should be removed.
 *
 * @return 0 if the thread was removed. -1 if the thread or group does not
 *         exists.
 */
int
thread_remove (thread_group * head, int group_id, int thread_id)
{
  thread *pt;
  thread *prev;
  thread_group *pg;

  /* Find the group. */
  pg = head;
  while (pg != NULL && pg->id != group_id)
    {
      pg = pg->next;
    }
  LOG_ERR_IF_RETURN (pg == NULL, -1, "Bad group id %d", group_id);

  /* Find thread. */
  pt = pg->first;
  prev = NULL;
  while (pt != NULL && pt->id != thread_id)
    {
      prev = pt;
      pt = pt->next;
    }
  LOG_ERR_IF_RETURN (pt == NULL, -1, "Bad thread id %d in group id %d",
		     thread_id, group_id);

  /* Remove thread. */
  if (prev == NULL)
    {
      pg->first = pt->next;
    }
  else
    {
      prev->next = pt->next;
    }

  thread_clear (pt);
  free (pt);

  DINFO (3, "Thread %d removed from %d", thread_id, group_id);

  return 0;
}

/**
 * @brief Remove all threads.
 *
 * Remove all threads of a group.
 *
 * @param first First thread that should be removed. Will be set to NULL.
 */
void
thread_remove_all (thread ** first)
{
  thread *next;
  thread *p;

  assert (first);

  p = *first;
  while (p != NULL)
    {
      next = p->next;
      thread_clear (p);
      free (p);
      p = next;
    }
  *first = NULL;

  DINFO (3, "All threads removed");
}

/**
 * @brief Set the threads to running or not.
 *
 * Set the specified threads to [not] running. If both \a group_id and
 * \a thread_id are -1 all threads in all groups will be set to the
 * running state.
 *
 * @param head The thread groups.
 * @param group_id The thread group id.
 * @param thread_id The thread id.
 * @param running Set to 1 if the thread (s) is running.
 * @param core The core thre thread was/is running on.
 *
 * @return 0 if setting was ok otherwise -1.
 */
int
thread_set_running (thread_group * head, int group_id, int thread_id,
		    int running, int core)
{
  thread_group *pg;
  thread *pt;

  LOG_ERR_IF_RETURN (running != 0 && running != 1, -1,
		     "running should be 1 or 0");

  if (head != NULL && group_id == -1 && thread_id == -1)
    {
      DINFO (3, "Setting all threads to %srunning", running ? "" : "not ");
      /* Set all threads to running. */
      pg = head;
      while (pg != NULL)
	{
	  pt = pg->first;
	  while (pt != NULL)
	    {
	      pt->running = running;
	      pt->core = core;
	      pt = pt->next;
	    }
	  pg = pg->next;
	}
      return 0;
    }

  pg = head;
  while (pg && pg->id != group_id)
    {
      pg = pg->next;
    }
  LOG_ERR_IF_RETURN (pg == NULL, -1, "No matching group %d", group_id);
  pt = pg->first;
  while (pt && pt->id != thread_id)
    {
      pt = pt->next;
    }
  LOG_ERR_IF_RETURN (pt == NULL, -1, "No matching thread %d in group %d",
		     thread_id, group_id);
  pt->running = running;
  pt->core = core;
  DINFO (3, "Setting group %d id %d to %s on core %d", group_id, thread_id,
	 running == 1 ? "running" : "not running", core);
  return 0;
}

/**
 * @brief Clear frame content of a thread.
 *
 * Clear the content of a thread frame and release the resources.
 *
 * @param pt The thread.
 */
void
thread_clear (thread * pt)
{
  assert (pt);

  if (pt->frame.file != NULL)
    {
      free (pt->frame.file);
      pt->frame.file = NULL;
    }
  if (pt->frame.fullname != NULL)
    {
      free (pt->frame.fullname);
      pt->frame.fullname = NULL;
    }
  if (pt->frame.func != NULL)
    {
      free (pt->frame.func);
      pt->frame.func = NULL;
    }
}

/*@}*/

/*******************************************************************************
 * library functions
 ******************************************************************************/

/**
 * @name Library functions.
 *
 * Libray helper functions. Functions for handle which library that are loaded.
 */
/*@{*/
/**
 * @brief Add a library.
 *
 * Add a loaded library.
 *
 * @param head A pointer to the first library, or NULL if no libraries are
 *             loaded.
 * @param id Id of the libraryi. Must not be NULL or an empty string.
 * @param target The target name of the library.
 * @param host The host name of the target.
 * @param loaded 1 if the symbols are loaded.
 *
 * @return 0 if successful otherwise -1;
 */
int
library_add (library ** head, const char *id, const char *target,
	     const char *host, int loaded)
{
  library *lp;

  assert (head);

  LOG_ERR_IF_RETURN (id == NULL || strlen (id) == 0, -1, "No id.");
  LOG_ERR_IF_RETURN (loaded != 0 && loaded != 1, -1,
		     "Loaded neither 0 or 1 but %d", loaded);

  lp = (library *) malloc (sizeof (*lp));
  LOG_ERR_IF_FATAL (lp == NULL, ERR_MSG_CREATE ("library"));

  lp->id = strdup (id);
  LOG_ERR_IF_FATAL (lp == NULL, ERR_MSG_CREATE ("String"));

  lp->host_name = strdup (host == NULL ? "" : host);
  LOG_ERR_IF_FATAL (lp == NULL, ERR_MSG_CREATE ("String"));

  lp->target_name = strdup (target == NULL ? "" : target);
  LOG_ERR_IF_FATAL (lp == NULL, ERR_MSG_CREATE ("String"));

  lp->symbols_loaded = loaded;
  lp->next = *head;
  *head = lp;

  DINFO (3, "library loaded (%d) '%s'", loaded, id);

  return 0;
}

/**
 * @brief Removes a library.
 *
 * Removes a library.
 *
 * @param head The first library.
 * @param id The id of the library to be unloaded.
 * @param target The target name of the library to be unloaded.
 * @param host The host name of the library to be unloaded.
 */
void
library_remove (library ** head, const char *id, const char *target,
		const char *host)
{
  library *p;
  library *prev = NULL;

  assert (head);
  assert (id);

  p = *head;
  while (p != NULL)
    {
      if (strcmp (p->id, id) == 0
	  && ((target == NULL && *(p->target_name) == '\0')
	      || (target != NULL && strcmp (p->target_name, target) == 0))
	  && ((host == NULL && *(p->host_name) == '\0')
	      || (host != NULL && strcmp (p->host_name, host) == 0)))
	{
	  break;
	}
      prev = p;
      p = p->next;
    }
  if (p == NULL)
    {
      LOG_ERR_IF (p == NULL, "Could not find library %s - %s - %s", id,
		  target == NULL ? "[TARGET NAME]" : target,
		  host == NULL ? "[HOST NAME]" : host);
      return;
    }

  if (prev == NULL)
    {
      *head = p->next;
    }
  else
    {
      prev->next = p->next;
    }
  if (p->host_name != NULL)
    {
      free (p->host_name);
    }
  if (p->target_name != NULL)
    {
      free (p->target_name);
    }
  if (p->id != NULL)
    {
      free (p->id);
    }

  free (p);

  DINFO (3, "library removed '%s'", id);
}

/**
 * @brief Removes all libraries.
 *
 * Removes and free all libraries.
 *
 * @param head The first library to be removed. Head will be set to NULL.
 */
void
library_remove_all (library ** head)
{
  library *p;
  library *next;

  assert (head);

  p = *head;
  while (p != NULL)
    {
      next = p->next;
      if (p->host_name != NULL)
	{
	  free (p->host_name);
	}
      if (p->target_name != NULL)
	{
	  free (p->target_name);
	}
      if (p->id != NULL)
	{
	  free (p->id);
	}
      free (p);
      p = next;
    }
  *head = NULL;

  DINFO (3, "All libraries unloaded");
}

/*@}*/

/*******************************************************************************
 * stack functions
 ******************************************************************************/

/**
 * @name Stack functions.
 *
 * Stack helper functions. Functions for handle the stack information of the
 * running/stopped threads.
 */
/*@{*/
/**
 * @brief Create a stack object.
 *
 * Create a stack object with initial depth of @a depth.
 *
 * @param depth The depth.
 *
 * @return A pointer to the new stack. Will exit if the allocation failed.
 */
stack *
stack_create (int depth)
{
  stack *new_stack;
  int i;

  assert (depth > 0);

  DINFO (1, "Creating stack of depth %d", depth);

  new_stack = (stack *) malloc (sizeof (*new_stack));
  LOG_ERR_IF_FATAL (new_stack == NULL, ERR_MSG_CREATE ("stack"));

  new_stack->core = -1;
  new_stack->depth = -1;
  new_stack->max_depth = depth;
  new_stack->thread_id = -1;
  new_stack->stack = (frame *) malloc (depth * sizeof (frame));
  LOG_ERR_IF_FATAL (new_stack == NULL, ERR_MSG_CREATE ("frames"));

  for (i = 0; i < depth; i++)
    {
      new_stack->stack[i].args = NULL;
      new_stack->stack[i].variables = NULL;
      new_stack->stack[i].file = NULL;
      new_stack->stack[i].func = NULL;
      new_stack->stack[i].fullname = NULL;
      stack_clean_frame (new_stack, i);
    }

  return new_stack;
}

/**
 * @brief Free stack.
 *
 * Free a stack and it's resources.
 *
 * @param stack The stack to be set free.
 */
void
stack_free (stack * stack)
{
  assert (stack);

  DINFO (1, "Freeing stack");

  stack_clean_frame (stack, -1);
  free (stack->stack);

  free (stack);
}

/**
 * @brief Get a frame from the stack.
 *
 * Get the frame of level @a level from the stack.
 *
 * @param stack The stack.
 * @param level The level.
 *
 * @return Pointer to the frame. Will never return NULL as it will exit
 *         if failing to allocate a frame.
 */
frame *
stack_get_frame (stack * stack, int level)
{
  int i;

  assert (stack);

  if (level >= stack->max_depth)
    {
      stack->stack = (frame *) realloc (stack->stack,
					(level +
					 FRAME_INCREASE) * sizeof (frame));
      LOG_ERR_IF_FATAL (stack->stack == NULL, ERR_MSG_CREATE ("frame"));

      for (i = stack->max_depth; i < level + FRAME_INCREASE; i++)
	{
	  stack->stack[i].args = NULL;
	  stack->stack[i].variables = NULL;
	  stack->stack[i].file = NULL;
	  stack->stack[i].func = NULL;
	  stack->stack[i].fullname = NULL;
	  stack_clean_frame (stack, i);
	}
      stack->max_depth = level + FRAME_INCREASE;

      DINFO (1, "Increased stack level to %d", stack->max_depth);
    }

  return &stack->stack[level];
}

/**
 * @brief Clear a frame in a stack.
 *
 * Clear a frame with depth @a depth and free it's resources. The frame it
 * self is kept and it's value are set to 0.
 *
 * @param stack The stack.
 * @param level The level of the frame.
 */
void
stack_clean_frame (stack * stack, int level)
{
  int istart;
  int istop;

  assert (stack);

  if (level >= stack->max_depth)
    {
      return;
    }

  if (level < 0)
    {
      istart = 0;
      istop = stack->max_depth;
      stack->depth = 0;
    }
  else
    {
      istart = level;
      istop = level + 1;
    }

  DINFO (5, "Clearing stack level %d->%d", istart, istop);

  for (level = istart; level < istop; level++)
    {
      DINFO (10, "Clearing level %d", level);
      variable_delete_list (stack->stack[level].args);
      variable_delete_list (stack->stack[level].variables);
      stack->stack[level].args = NULL;
      stack->stack[level].variables = NULL;
      if (stack->stack[level].file != NULL)
	{
	  free (stack->stack[level].file);
	  stack->stack[level].file = NULL;
	}
      if (stack->stack[level].fullname != NULL)
	{
	  free (stack->stack[level].fullname);
	  stack->stack[level].fullname = NULL;
	}
      if (stack->stack[level].func != NULL)
	{
	  free (stack->stack[level].func);
	  stack->stack[level].func = NULL;
	}
      stack->stack[level].addr = -1;
      stack->stack[level].line = -1;
    }
}

/**
 * @brief Create and insert a variable.
 *
 * Creates a new variable or argument and inserts it in the frame. If
 * there already is a variable with the @a name the value is updated if
 * the @a type matches if both types are non NULL. If the @a type does not
 * match -1 is returned. If the new type is non NULL, but the type in the
 * list are NULL, we will set the new type. The reason is that the debugger
 * does not always send a type.
 *
 * If @a variable in non 0, the new variable is inserted in the variable list.
 * If the @a variable is 0 the new variable is inserted in the frames args list.
 *
 * @param frame The frame to insert the variable in.
 * @param name The name of the variable.
 * @param type The type of the variable.
 * @param value The value of the variable.
 * @param var If 0 the new variable is considered as a argument. Otherwise
 *                 it is considered as a variable.
 * @param back If 1 the variable is appendended to the list of variables.
 *
 * @return 0 if the successful otherwise -1.
 */
int
frame_insert_variable (frame * frame, char *name, char *type, char *value,
		       int var, int back)
{
  variable *pv;
  variable *new_variable;

  assert (frame);
  assert (name);

  LOG_ERR_IF_RETURN (type == NULL
		     && value == NULL, -1, "Neither type or value");
  if (var)
    {
      pv = frame->variables;
    }
  else
    {
      pv = frame->args;
    }
  while (pv != NULL && strcmp (pv->name, name) != 0)
    {
      pv = pv->next;
    }
  if (pv != NULL)
    {
      /*
       * We already have the variable/arg so update the value, and the type if
       * the new type is non NULL and the old type is NULL. If both types are
       * non NULL, check if they match.
       */
      LOG_ERR_IF_RETURN (pv->type != NULL && type != NULL
			 && strcmp (pv->type, type) != 0, -1,
			 "Type '%s' does not match '%s'", type, pv->type);
      if (value == NULL)
	{
	  if (pv->value)
	    {
	      free (pv->value);
	      pv->value = NULL;
	    }
	}
      else
	{
	  if (pv->value)
	    {
	      if (strcmp (pv->value, value) != 0)
		{
		  if (strlen (pv->value) >= strlen (value))
		    {
		      sprintf (pv->value, "%s", value);
		    }
		  else
		    {
		      free (pv->value);
		      pv->value = strdup (value);
		      LOG_ERR_IF_FATAL (pv->value == NULL,
					ERR_MSG_CREATE ("string"));
		    }
		}
	    }
	  else
	    {
	      pv->value = strdup (value);
	      LOG_ERR_IF_FATAL (pv->value == NULL, ERR_MSG_CREATE ("string"));
	    }
	}

      if (type != NULL && pv->type == NULL)
	{
	  pv->type = strdup (type);
	  LOG_ERR_IF_FATAL (pv->value == NULL, ERR_MSG_CREATE ("string"));
	}
      DINFO (3, "Updated %s %s %s = %s", var == 1 ? "variable" : "argument",
	     pv->name, pv->type, pv->value ? pv->value : "[MIA]");
      return 0;
    }

  /* Create a new variable. */
  new_variable = (variable *) malloc (sizeof (*new_variable));
  LOG_ERR_IF_RETURN (new_variable == NULL, -1, ERR_MSG_CREATE ("variable"));
  new_variable->name = strdup (name);
  LOG_ERR_IF_RETURN (new_variable->name == NULL, -1,
		     ERR_MSG_CREATE ("string"));

  if (type != NULL)
    {
      new_variable->type = strdup (type);
      LOG_ERR_IF_RETURN (new_variable->type == NULL, -1,
			 ERR_MSG_CREATE ("string"));
    }
  else
    {
      new_variable->type = NULL;
    }
  if (value != NULL)
    {
      new_variable->value = strdup (value);
      LOG_ERR_IF_RETURN (new_variable->value == NULL, -1,
			 ERR_MSG_CREATE ("string"));
    }
  else
    {
      new_variable->value = NULL;
    }

  DINFO (3, "New %s %s %s = %s", var == 1 ? "variable" : "argument",
	 type ? type : "[TYPE]", name, value ? value : "[VALUE]");

  if (back == 1)
    {
      if (var)
	{
	  pv = frame->variables;
	}
      else
	{
	  pv = frame->args;
	}
      while (pv != NULL && pv->next != NULL)
	{
	  pv = pv->next;
	}
      if (pv != NULL)
	{
	  pv->next = new_variable;
	  new_variable->next = NULL;
	  return 0;
	}
    }

  if (var)
    {
      new_variable->next = frame->variables;
      frame->variables = new_variable;
    }
  else
    {
      new_variable->next = frame->args;
      frame->args = new_variable;
    }

  return 0;
}

/**
 * @brief Delete a list of variables.
 *
 * Delete a list of variables and free the resources.
 *
 * @param var_list List of variables.
 */
void
variable_delete_list (variable * var_list)
{
  variable *p;
  variable *next;

  p = var_list;
  while (p)
    {
      next = p->next;
      if (p->name != NULL)
	{
	  free (p->name);
	}
      if (p->type != NULL)
	{
	  free (p->type);
	}
      if (p->value != NULL)
	{
	  free (p->value);
	}
      free (p);
      p = next;
    }
}

/*@}*/

/*******************************************************************************
 * Assembler functions
 ******************************************************************************/
/**
 * @name Assembler functions.
 *
 * Assembler line helper functions.
 *
 * Functions for storing source line and assmbler line for the current thread.
 */
/*@{*/
/**
 * @brief Create an assembler.
 *
 * Creates an assembler object, which holds the assembler lines of a function.
 *
 * @return Pointer to the assembler object.
 */
assembler *
ass_create (void)
{
  assembler *fat_ass;

  fat_ass = (assembler *) malloc (sizeof (*fat_ass));
  LOG_ERR_IF_FATAL (fat_ass == NULL, "Memory");
  memset (fat_ass, 0, sizeof (*fat_ass));

  return fat_ass;
}

/**
 * @brief Free the assembler.
 *
 * Free the assembler and it's resources.
 *
 * @param ass The sorry ass to be set free.
 */
void
ass_free (assembler * ass)
{
  asm_line *p;
  asm_line *next;
  src_line *s;
  src_line *sn;

  assert (ass);

  /* Clear src pool. */
  s = ass->src_pool;
  while (s)
    {
      sn = s->next;
      free (s);
      s = sn;
    }

  /* Clear asm pool */
  p = ass->pool;
  while (p)
    {
      if (p->inst)
	{
	  free (p->inst);
	}
      next = p->next;
      free (p);
      p = next;
    }

  /* Clear all in use src lines */
  s = ass->lines;
  while (s)
    {
      p = s->lines;
      while (p)
	{
	  if (p->inst)
	    {
	      free (p->inst);
	    }
	  next = p->next;
	  free (p);
	  p = next;
	}
      sn = s->next;
      free (s);
      s = sn;
    }

  /* Clear function & file. */
  if (ass->function)
    {
      free (ass->function);
    }
  if (ass->file)
    {
      free (ass->file);
    }
  free (ass);
}

/**
 * @brief Reset the ass lines.
 *
 * Reset the lines without releasing any resources.
 *
 * @param ass The sorry ass.
 */
void
ass_reset (assembler * ass)
{
  asm_line *pal;
  asm_line *alnext;
  src_line *psl;
  src_line *slnext;

  psl = ass->lines;
  while (psl)
    {
      pal = psl->lines;
      while (pal)
	{
	  alnext = pal->next;
	  pal->next = ass->pool;
	  ass->pool = pal;
	  pal = alnext;
	}
      psl->lines = NULL;
      slnext = psl->next;
      psl->next = ass->src_pool;
      ass->src_pool = psl;
      psl = slnext;
    }
  ass->current_line = NULL;
  ass->lines = NULL;
  if (ass->function)
    {
      ass->function[0] = '\0';
    }

}

/**
 * @brief Insert a new asm line.
 *
 * Insert a new asm line in the assembler. If it belongs to a new the function
 * the old lines are removed.
 *
 * @param ass The assembler lines.
 * @param file The name of the source file.
 * @param func The function name.
 * @param line_nr The source file line number.
 * @param address The address of the line.
 * @param offset The offset from function start.
 * @param inst The instruction.
 *
 * @return 0 if the new line is in the old function. 1 if it belongs to a new
 *         function.
 */
int
ass_add_line (assembler * ass, const char *file, const char *func,
	      int line_nr, int address, int offset, const char *inst)
{
  char *t;
  int ret = 0;
  asm_line *pal;
  asm_line *qal;
  asm_line *preval;
  src_line *psl;
  src_line *qsl;
  src_line *prevsl;

  assert (ass);
  assert (file);

  if ((ass->function && func && strcmp (func, ass->function) != 0)
      || (ass->function == NULL && func)
      || (address - offset != ass->address && address > 0)
      || (ass->file && strcmp (file, ass->file) != 0))
    {
      DINFO (3, "New function %s 0x%0X-> %s 0x%0X", ass->function,
	     ass->address, func, address);
      /* Move all lines to pools */
      psl = ass->lines;
      while (psl)
	{
	  pal = psl->lines;
	  while (pal)
	    {
	      qal = pal->next;
	      pal->next = ass->pool;
	      ass->pool = pal;
	      pal = qal;
	    }
	  psl->last = NULL;
	  psl->lines = NULL;
	  qsl = psl->next;
	  psl->next = ass->src_pool;
	  ass->src_pool = psl;
	  psl = qsl;
	}
      ass->current_line = NULL;
      ass->lines = NULL;

      /* Update file && function. */
      LPRINT (ass->file, 1, ass->file_size, "%s", file);
      LOG_ERR_IF_FATAL (ass->file == NULL, ERR_MSG_CREATE ("file"));
      LPRINT (ass->function, 1, ass->size, "%s", func ? func : "");
      LOG_ERR_IF_FATAL (ass->function == NULL, ERR_MSG_CREATE ("function"));
      ass->address = address;
      ret = 1;
    }

  /* Check if it belongs to a new source line. */
  if (ass->current_line && ass->current_line->line_nr == line_nr)
    {
      /* same src file line number. */
      DINFO (5, "Same src line");
      psl = ass->current_line;
    }
  else
    {
      /* Get a new src line */
      if (ass->src_pool == NULL)
	{
	  psl = (src_line *) malloc (sizeof (*psl));
	  LOG_ERR_IF_FATAL (psl == NULL, ERR_MSG_CREATE ("src line"));
	}
      else
	{
	  psl = ass->src_pool;
	  ass->src_pool = psl->next;
	  psl->next = NULL;
	}
      psl->lines = NULL;
      psl->next = NULL;
      psl->line_nr = line_nr;
      psl->last = NULL;

      /* Insert the src line in order. */
      if (ass->current_line && ass->current_line->line_nr < psl->line_nr
	  && ass->current_line->next == NULL)
	{
	  prevsl = ass->current_line;
	  qsl = ass->current_line->next;
	}
      else
	{
	  prevsl = NULL;
	  qsl = ass->lines;
	}
      while (qsl && qsl->line_nr < psl->line_nr)
	{
	  prevsl = qsl;
	  qsl = qsl->next;
	}
      psl->next = qsl;
      if (prevsl != NULL)
	{
	  prevsl->next = psl;
	}
      else
	{
	  ass->lines = psl;
	}
      ass->current_line = psl;
    }

  if (func == NULL)
    {
      DINFO (3, "Add empt src line %s %d %s", file, line_nr, func);
      /* No asm lines. */
      return ret;
    }

  /* Get a new asm line */
  if (ass->pool != NULL)
    {
      pal = ass->pool;
      ass->pool = pal->next;
    }
  else
    {
      pal = (asm_line *) malloc (sizeof (*pal));
      pal->inst = 0;
      pal->size = 0;
    }
  pal->address = address;
  pal->offset = offset;
  pal->next = NULL;

  if (psl->last && pal->address > psl->last->address
      && psl->last->next == NULL)
    {
      psl->last->next = pal;
    }
  else
    {
      qal = psl->lines;
      preval = NULL;
      while (qal && qal->address < pal->address)
	{
	  preval = qal;
	  qal = qal->next;
	}
      pal->next = qal;

      if (preval == NULL)
	{
	  psl->lines = pal;
	}
      else
	{
	  preval->next = pal;
	}
    }
  t = pal->inst;
  if (t == NULL)
    {
      pal->inst = strdup (inst ? inst : "");
      pal->size = strlen (pal->inst) + 1;
    }
  else
    {
      LPRINT (pal->inst, 1, pal->size, "%s", inst);
    }
  LOG_ERR_IF_FATAL (pal->inst == NULL, ERR_MSG_CREATE ("line"));
  psl->last = pal;

  DINFO (3, "Add src line %s %d %s: 0x%0X %s", file, line_nr, func, address,
	 inst);

  return ret;
}

/*@}*/

/*******************************************************************************
 * Register functions
 ******************************************************************************/
/**
 * @name Register functions.
 *
 * Register function.
 *
 * Functions for holding information of the registers.
 */
/*@{*/
/**
 * @brief Create register object.
 *
 * Create a register object to handle all registers.
 *
 * @return A pointer to the new object.
 */
data_registers *
data_registers_create (void)
{
  data_registers *registers;

  registers = (data_registers *) malloc (sizeof (*registers));
  LOG_ERR_IF_FATAL (registers == NULL, ERR_MSG_CREATE ("registers"));
  memset (registers, 0, sizeof (*registers));

  registers->registers = (data_reg *) malloc (128 * sizeof (data_reg));
  LOG_ERR_IF_FATAL (registers->registers == NULL,
		    ERR_MSG_CREATE ("registers"));
  registers->size = 128;
  registers->len = 0;

  memset (registers->registers, 0, 128 * sizeof (data_reg));

  return registers;
}

/**
 * @brief Free register object.
 *
 * Free a register object and it's resources.
 *
 * @param registers The registers.
 */
void
data_registers_free (data_registers * registers)
{
  int i;

  for (i = 0; i < registers->size; i++)
    {
      if (registers->registers[i].reg_name)
	{
	  free (registers->registers[i].reg_name);
	}
      if (registers->registers[i].type == 2 && registers->registers[i].svalue)
	{
	  free (registers->registers[i].svalue);
	}
    }
  free (registers->registers);

  free (registers);
}

/**
 * @brief Add a register.
 *
 * Add a register to the register object.
 *
 * @param registers The registers.
 * @param nr The number associated with the new register.
 * @param name The name of the register.
 */
void
data_registers_add (data_registers * registers, int nr, const char *name)
{
  data_reg *t;

  assert (registers);

  if (nr >= registers->size)
    {
      registers->size += 10;
      t = (data_reg *) realloc (registers->registers,
				registers->size * sizeof (data_reg));
      LOG_ERR_IF_FATAL (t == NULL, ERR_MSG_CREATE ("registers"));
      memset (&t[registers->size - 10], 0, 10 * sizeof (data_reg));
      registers->registers = t;
    }

  if (registers->registers[nr].reg_name != NULL)
    {
      free (registers->registers[nr].reg_name);
    }
  if (nr + 1 > registers->len)
    {
      registers->len = nr + 1;
    }

  registers->registers[nr].reg_name = strdup (name);
  LOG_ERR_IF_FATAL (registers->registers[nr].reg_name == NULL,
		    ERR_MSG_CREATE ("register"));
  registers->registers[nr].type = 2;
  registers->registers[nr].svalue = NULL;
  registers->registers[nr].size = 0;
}

/**
 * @brief Set a value of a register.
 *
 * Set a value of a register.
 *
 * @param registers The register object.
 * @param nr The register number to be set.
 * @param value The value.
 *
 * @return 0 upon success. -1 if the register does not exists.
 */
int
data_registers_set_value (data_registers * registers, int nr, uint64_t value)
{
  assert (registers);

  LOG_ERR_IF_RETURN (nr < 0 || nr >= registers->size, -1,
		     "Reg nr %d out out bounds", nr);

  if (registers->registers[nr].type == 2 && registers->registers[nr].svalue)
    {
      free (registers->registers[nr].svalue);
      registers->registers[nr].size = 0;
    }
  registers->registers[nr].type = 0;
  registers->registers[nr].u64 = value;

  return 0;
}

/**
 * @brief Set a value of a register.
 *
 * Set a value of a register. The value is a string.
 *
 * @param registers The register object.
 * @param nr The register number to be set.
 * @param value The value.
 *
 * @return 0 upon success. -1 if the register does not exists.
 */
int
data_registers_set_str_value (data_registers * registers, int nr,
			      const char *value)
{
  assert (registers);

  DINFO (5, "Setting reg %d %s to %s", nr, nr >= 0 && nr < registers->len ?
	 registers->registers[nr].reg_name : "", value);
  LOG_ERR_IF_RETURN (nr < 0 || nr >= registers->size, -1,
		     "Reg nr %d out out bounds", nr);

  if (registers->registers[nr].type != 2)
    {
      registers->registers[nr].svalue = NULL;
      registers->registers[nr].size = 0;
    }
  registers->registers[nr].type = 2;
  LPRINT (registers->registers[nr].svalue, 1, registers->registers[nr].size,
	  "%s", value);

  return 0;
}

/*@}*/
