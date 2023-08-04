; CGARGB.DRV - An SCI video driver for the CGA
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
driver_name     db      6, "cgargb"
description     db      18, "CGA RGB - 4 Colors"

%include "cgacommon.i"

%include "cgargb_tables.i"

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

        ; set video mode 5 (320x200 - 4 colors red/cyan)
        mov     ax,5
        int     10h
		
		; set intense palette
		mov dx, 0x3d9
		mov al, 0x30
		out dx, al

		; enable red/cyan palette for EGA/VGA cards
		; set red
		mov ax, 0x1000
		mov bx, 0x3c02
		int 0x10

		; set cyan
		mov ax, 0x1000
		mov bx, 0x3b01
		int 0x10

		; set white
		mov ax, 0x1000
		mov bx, 0x3f03
		int 0x10

        ; restore mode number
        pop     ax
        xor     ah,ah

        ret
