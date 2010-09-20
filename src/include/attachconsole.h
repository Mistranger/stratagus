/*
    attachconsole.h - WINAPI AttachConsole
    Copyright (C) 2009-2010  Pali Rohár <pali.rohar@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef WIN32

/// Try attach console of parent process for std input/output in Windows NT, 2000, XP or new
bool WINAPI_AttachConsole();

#else

bool WINAPI_AttachConsole() { return false; }

#endif //WIN32
