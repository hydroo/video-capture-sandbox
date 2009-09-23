/* videocapture is a tool with no special purpose
 *
 * Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef PREREQS_HPP
#define PREREQS_HPP


/* more to come */


#ifdef HAVE_VAMPIRTRACE

    #include <vt_user.h>

    /** write a trace record at his point and the destructor call/block end
        @note needs HAVE_VAMPIRTRACE to be defined */
    #define VTN(name) VT_TRACER(name);
    /** write an enter record - "trace start"
        @note needs HAVE_VAMPIRTRACE to be defined
        @ingroup prereqs */
    #define VTN_START(name) VT_USER_START(name)
    /** write a leave record - "trace end"
        @note needs HAVE_VAMPIRTRACE to be defined */
    #define VTN_END(name) VT_USER_END(name)

    /** trace this function - from this point to desctructor call/block end
        @note needs HAVE_VAMPIRTRACE to be defined */
    #define VT VTN(__func__)
    /** trace this function - start
        @note needs HAVE_VAMPIRTRACE to be defined */
    #define VT_START VTN_START(__func__)
    /** trace this function - end
        @note needs HAVE_VAMPIRTRACE to be defined */
    #define VT_END VTN_END(__func__)

#else /* HAVE_VAMPIRTRACE */

    #define VTN(name)
    #define VTN_START(name)
    #define VTN_END(name)

    #define VT
    #define VT_START
    #define VT_END

#endif /* HAVE_VAMPIRTRACE */


#endif /* PREREQS_HPP */

