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
 * @file pseudo_fork.h
 *
 * @brief Start a program in a pseudo terminal.
 *
 * Start a program in a pseudo terminal.
 *
 * @todo move to misc?!
 *
 */
#ifndef PSEUDO_FORK_H
#define PSEUDO_FORK_H

/*******************************************************************************
 * Public functions
 ******************************************************************************/
int start_forkpty (int *fd, pid_t * cpid, char *debugger, char **args);
#endif
