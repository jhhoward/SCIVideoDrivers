; CGACOMP.DRV - An SCI video driver for CGA composite mode
; Copyright (C) 2020  James Howard
; Based on PCPLUS.DRV
; Copyright (C) 2020  Benedikt Freisen
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
;
; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

; SCI drivers use a single code/data segment starting at offset 0
[bits 16]
[org 0]

;-------------- entry --------------------------------------------------
; This is the driver entry point that delegates the incoming far-call
; to the dispatch routine via jmp.
;
; Parameters:   bp      index into the call table (always even)
;               ?       depends on the requested function
; Returns:      ?       depends on the requested function
;-----------------------------------------------------------------------
entry:  jmp     dispatch

; magic numbers followed by two pascal strings
signature       db      00h, 21h, 43h, 65h, 87h, 00h
driver_name     db      7, "cgacomp"
description     db      25, "CGA Composite - 16 Colors"

%include "cgacommon.i"

%include "cgacomp_tables.i"

;-------------- init_video_mode-----------------------------------------
; Initializes the video mode provided by this driver and returns the
; previous video mode, i.e. the BIOS mode number.
;
; Parameters:   --
; Returns:      ax      BIOS mode number of the previous mode
;-----------------------------------------------------------------------
init_video_mode:
        ; get current video mode
        mov     ah,0fh
        int     10h

        ; save mode number
        push    ax

        ; set video mode 6 (640x200 - 2 colors)
        mov     ax,6
        int     10h

		; enable colour burst
		mov dx, 0x3d8
		mov al, 0x1a
		out dx, al

        ; restore mode number
        pop     ax
        xor     ah,ah

        ret
