/*
  Hatari - screenConvert.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

void Screen_GenConvert(uint32_t vaddr, int vw, int vh, int vbpp, int nextline,
                       int leftBorderSize, int rightBorderSize,
                       int upperBorderSize, int lowerBorderSize);
