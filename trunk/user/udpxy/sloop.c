/* @(#) wrapper for udpxy server loop
 *
 * Copyright 2008-2012 Pavel V. Cherenkov (pcherenkov@gmail.com)
 *
 *  This file is part of udpxy.
 *
 *  udpxy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  udpxy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with udpxy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/syscall.h>

/* need to know if pselect(2) is available */
#if defined(SYS_pselect6)
    #define HAS_PSELECT 1
#endif

/* use pselect(2) in server loop unless select(2) demanded */
#if defined(HAS_PSELECT) && !defined(USE_SELECT)
    #define USE_PSELECT 1
#endif

#ifdef USE_PSELECT
    #include "sloop_p.c"
#else
    #include "sloop_s.c"
#endif

/* __EOF__ */

