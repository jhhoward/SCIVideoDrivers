; Common CGA driver functions
; Copyright (C) 2023  James Howard
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

; call-table for the dispatcher
call_tab        dw      get_color_depth         ; bp = 0
                dw      init_video_mode         ; bp = 2
                dw      restore_mode            ; bp = 4
                dw      update_rect             ; bp = 6
                dw      show_cursor             ; bp = 8
                dw      hide_cursor             ; bp = 10
                dw      move_cursor             ; bp = 12
                dw      load_cursor             ; bp = 14
                dw      shake_screen            ; bp = 16
                dw      scroll_rect             ; bp = 18
				dw		set_palette				; bp = 20 V_SETPALETTE
				dw		dummy_fn				; bp = 22 V_GETPALETTE
				dw		dummy_fn				; bp = 24 V_SETPALETTE_CYCLE
				dw		dummy_fn				; bp = 26 V_RECT_DISPLAY
				dw		dummy_fn				; bp = 28 V_SETMODE
				dw		dummy_fn				; bp = 30 V_DISPLAYPAGE
				dw		dummy_fn				; bp = 32 V_COPYPAGE
				dw		dummy_fn				; bp = 34 V_SETUPVMAP
				dw		dummy_fn				; bp = 36 V_HWSHIFT
				dw		dummy_fn				; bp = 38 V_SETOUTPAGE
				dw		dummy_fn				; bp = 40 V_GETOUTPAGE
				dw		dummy_fn				; bp = 42 V_CLEARPAGEMODE

; active mouse cursor in the internal pixel format
; five bytes per line (four bytes mask data + one padding byte)
cursor_and      times   80 db 0                 ; inverted and-mask
cursor_or       times   80 db 0                 ; or-mask

; saved background pixels overwritten by the cursor
cursor_bg       times   160 db 0
cursor_ofs      dw      0
cursor_hbytes   db      0
cursor_rows     db      0

cursor_counter  dw      0
cursor_x        dw      0
cursor_y        dw      0
cursor_new_x    dw      0
cursor_new_y    dw      0

cursor_lock     dw      0


; Converts from 8 bit palette value to CGA pattern	
convert_palette		times 256 dw 0				
	
framebuffer_segment_cache	dw	0

	
;-------------- dispatch -----------------------------------------------
; This is the dispatch routine that delegates the incoming far-call to
; to the requested function via call.
;
; Parameters:   bp      index into the call table (always even)
;               ?       depends on the requested function
; Returns:      ?       depends on the requested function
;-----------------------------------------------------------------------
dispatch:
        ; save segments & set ds to cs
        push    es
        push    ds
        push    cs
        pop     ds

        ; dispatch the call while preserving ax, bx, cx, dx and si
        call    [cs:call_tab+bp]

        ; restore segments
        pop     ds
        pop     es

        retf

;-------------- get_color_depth ----------------------------------------
; Returns the number of colors supported by the driver, e.g. 4 or 16.
;
; Parameters:   --
; Returns:      ax      number of colors
; Notes:        The PC1512 driver returns the word -1, instead.
;-----------------------------------------------------------------------
get_color_depth:
        mov     ax,4
        ret

;-------------- restore_mode -------------------------------------------
; Restores the provided BIOS video mode.
;
; Parameters:   ax      BIOS mode number
; Returns:      --
;-----------------------------------------------------------------------
restore_mode:
        ; save parameter
        push    ax

        ; disable ColorPlus extensions
        mov     dx,3ddh
        mov     al,0
        out     dx,al

        ; restore parameter
        pop     ax

        ; set video mode
        xor     ah,ah
        int     10h

        ret

;-------------- update_rect --------------------------------------------
; Transfer the specified rectangle from the engine's internal linear
; frame buffer of IRGB pixels to the screen.
;
; Parameters:   ax      Y-coordinate of the top-left corner
;               bx      X-coordinate of the top-left corner
;               cx      Y-coordinate of the bottom-right corner
;               dx      X-coordinate of the bottom-right corner
;               si      frame buffer segment (offset = 0)
; Returns:      --
; Notes:        The implementation may expand the rectangle as needed
;               and may assume that all parameters are valid.
;               It has to hide the mouse cursor if it intersects with
;               the rectangle and has to lock it, otherwise.
;-----------------------------------------------------------------------
update_rect:
		mov		[framebuffer_segment_cache], si

        shr     bx,1
        shr     bx,1
        add     dx,3
        shr     dx,1
        shr     dx,1
        ; load and convert cursor x
        mov     bp,[cursor_x]
        shr     bp,1
        shr     bp,1
        ; compare to right edge
        cmp     dx,bp
        jl      .just_lock
        ; compare to left edge (a bit generously)
        add     bp,5
        sub     bp,bx
        jl      .just_lock
        ; load cursor y
        mov     bp,[cursor_y]
        ; compare to bottom edge
        cmp     cx,bp
        jl      .just_lock
        ; compare to top edge
        add     bp,16
        sub     bp,ax
        jl      .just_lock

        ; locking the cursor is not enough -> hide it
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        call    hide_cursor
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        clc
        jmp     .just_hide

.just_lock:
        call    lock_cursor
        stc
.just_hide:
        pushf

        mov     bp,0b800h
        mov     es,bp
        push    ds
        mov     ds,si

        ; calculate source address
        sub     cx,ax			; calculate height
        sub     dx,bx			; calculate width
        mov     bp,ax			; store top position in bp
        mov     ax,320
        mul     bp
        add     ax,bx
		add     ax,bx
		add     ax,bx
		add     ax,bx
        mov     si,ax
        ; calculate destination address
        mov     ax,bp
        xor     di,di
        shr     ax,1
        rcr     di,1
        shr     di,1
        shr     di,1
        mov     ah,80
        mul     ah
        add     di,ax
        add     di,bx

		mov     bx,convert_palette
		
        mov     dh,dl
        mov     dl,4
        mov     bp,dx
        mov     dx,cx
		
		
		;; hack to just redraw the whole screen
		xor		di,di
		xor		si,si
		mov		bp,05000h
		mov		dx,200
		;;
		mov 	cl, 0
		
.y_loop:
		mov ah, cl
        mov     cx,bp
		mov cl, ah
		
        push    si
        push    di
.x_loop:
        ; load four bytes from the engine's frame buffer
		xor		ah,ah

		; use conversion table for each byte
        lodsb
		cs      xlatb
		and		al,0c0h
		or		ah,al

        lodsb
		cs      xlatb
		and		al,30h
		or		ah,al

        lodsb
		cs      xlatb
		and		al,0ch
		or		ah,al

        lodsb
		cs      xlatb
		and		al,03h
		or		ah,al
		
		xchg    al,ah
		
        ; write to the screen's VRAM
        stosb
		
        dec     ch
        jnz    .x_loop

        pop     di
        pop     si
        add     si,320
		
		;mov 	cl, 4
		
        ; handle scanline interleaving
		add		bx, 256
        add     di,8192
        cmp     di,16384
        jb      .odd
        sub     di,16304
		sub		bx, 512
		
		;mov 	cl, 0
.odd:

        dec     dx
        jns     .y_loop

        pop     ds
        ; unlock/show cursor
        popf
        jnc     .show
        call    unlock_cursor
        ret
.show:  call    show_cursor

        ret

;-------------- show_cursor --------------------------------------------
; Increment the mouse cursor visibility counter and draw the cursor if
; the counter reaches one.
;
; Parameters:   --
; Returns:      --
;-----------------------------------------------------------------------
show_cursor:
        ; hard synchronization
        pushf
        cli

        or      word [cursor_counter],0
        jne     .skip
        call    draw_cursor

.skip:  inc     word [cursor_counter]
        popf

        ret

;-------------- hide_cursor --------------------------------------------
; Decrement the mouse cursor visibility counter and restore the
; background if the counter reaches zero.
;
; Parameters:   --
; Returns:      --
;-----------------------------------------------------------------------
hide_cursor:
        ; hard synchronization
        pushf
        cli

        dec     word [cursor_counter]
        jnz     .skip
        call    restore_background

.skip:  popf

        ret

;-------------- move_cursor --------------------------------------------
; Moves the mouse cursor, unless it is locked, in which case it will be
; moved when unlocked.
;
; Parameters:   ax      new X-coordinate
;               bx      new Y-coordinate
; Returns:      --
; Note:         This function has to preserve all registers not
;               otherwise preserved.
;-----------------------------------------------------------------------
move_cursor:
        ; save everything
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        pushf

        ; move the cursor, unless it is locked
        cli
        push    bx
        push    ax
        cmp     word [cursor_lock],0
        jnz     .skip
        call    hide_cursor
        pop     word [cursor_x]
        pop     word [cursor_y]
        call    show_cursor
        jmp     .end
.skip:
        ; if locked, save coordinates for later
        pop     word [cursor_new_x]
        pop     word [cursor_new_y]
.end:

        ; restore everything
        popf
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax

        ret

;-------------- load_cursor --------------------------------------------
; Loads a new graphical mouse cursor.
;
; Parameters:   ax      segment of the new cursor
;               bx      offset of the new cursor
; Returns:      ax      the current cursor visibility
; Notes:        Source cursor format expected by load_cursor:
;               Two unused words followed by an AND- and an OR-matrix,
;               each conststing of sixteen 16-bit little-endian words.
;               The most significant bit is the left-most pixel.
;-----------------------------------------------------------------------
load_cursor:
        ; copy the new cursor to the internal cursor data structure
        push    ds
        mov     ds,ax
        lea     si,[bx+4]
        mov     di,cursor_and
        mov     ax,cs
        mov     es,ax

        mov     dx,16
.loop_y_and:
        lodsw
        xchg    al,ah
        not     ax
        mov     cx,8
.shift_loop_1_and:
        shr     ax,1
        rcr     bx,1
        sar     bx,1
        loop    .shift_loop_1_and
        xchg    bl,bh
        mov     [es:di],bx
        inc     di
        inc     di
        mov     cx,8
.shift_loop_2_and:
        shr     ax,1
        rcr     bx,1
        sar     bx,1
        loop    .shift_loop_2_and
        mov     ax,bx
        xchg    al,ah
        stosw
        xor     ax,ax
        stosb
        dec     dx
        jnz    .loop_y_and

        mov     dx,16
.loop_y_or:
        lodsw
        xchg    al,ah
        mov     cx,8
.shift_loop_1_or:
        shr     ax,1
        rcr     bx,1
        sar     bx,1
        loop    .shift_loop_1_or
        xchg    bl,bh
        mov     [es:di],bx
        inc     di
        inc     di
        mov     cx,8
.shift_loop_2_or:
        shr     ax,1
        rcr     bx,1
        sar     bx,1
        loop    .shift_loop_2_or
        mov     ax,bx
        xchg    al,ah
        stosw
        xor     ax,ax
        stosb
        dec     dx
        jnz    .loop_y_or

        pop     ds

        ; make sure that the on-screen cursor changes, as well
        call    hide_cursor
        call    show_cursor

        ; return the cursor visibility counter
        mov     ax,[cursor_counter]

        ret

;-------------- shake_screen -------------------------------------------
; Quickly shake the screen horizontally and/or vertically by a few
; pixels to visualize collisions etc.
;
; Parameters:   ax      segment of timer tick word for busy waiting
;               bx      offset of timer tick word for busy waiting
;               cx      number of times (forth & back count separately)
;               dl      direction mask (bit 1: down; bit 2: right)
; Returns:      --
; Notes:        The implementation should use hardware scrolling and
;               eventually restore the original screen position.
;               The timer tick word is modified concurrently and should
;               be treated as read-only value within this function.
;               The CGA drivers shake by one CRTC character cell size
;               and wait for three timer ticks between steps, whereas
;               the MCGA driver provides an empty dummy function.
;-----------------------------------------------------------------------
shake_screen:
        ; this dummy implementation returns right away
        ret

;-------------- scroll_rect --------------------------------------------
; Scroll out the content of the specified rectangle while filling it
; with the new content.
;
; Parameters:   ax      Y-coordinate of the top-left corner
;               bx      X-coordinate of the top-left corner
;               cx      Y-coordinate of the bottom-right corner
;               dx      X-coordinate of the bottom-right corner
;               di      frame buffer segment (offset = 0)
;               ?       potentially further parameters for
;                       implementations that actually scroll
; Returns:      --
; Notes:        Simple implementations may omit the scrolling and
;               delegate the call to update_rect, adjusting the
;               parameters as needed.
;-----------------------------------------------------------------------
scroll_rect:
        ; update_rect expects the target frame buffer segment in si
        mov     si,di
        ; tail call to update_rect
        jmp     update_rect
		
dummy_fn:
		ret

;***********************************************************************
; The helper functions below are not part of the API.
;***********************************************************************

;-------------- draw_cursor --------------------------------------------
; Draws the mouse cursor after saving the screen content at its
; position to a buffer.
;
; Parameters:   --
; Returns:      --
;-----------------------------------------------------------------------
draw_cursor:
        ; calculate on-screen cursor dimensions
        mov     ax,200
        sub     ax,[cursor_y]
        cmp     ax,16
        jl      .nocrop_v
        mov     ax,16
.nocrop_v:
        mov     [cursor_rows],ax

        mov     ax,[cursor_x]
        shr     ax,1
        shr     ax,1
        mov     bx,ax
        sub     ax,80
        neg     ax
        cmp     ax,5
        jl      .nocrop_h
        mov     ax,5
.nocrop_h:
        mov     [cursor_hbytes],al

        ; calculate cursor offset in video ram
        mov     ax,[cursor_y]
        xor     si,si
        shr     ax,1
        rcr     si,1
        shr     si,1
        shr     si,1
        mov     ah,80
        mul     ah
        add     si,ax
        add     si,bx
        mov     [cursor_ofs],si

        ; save screen content that will be overwritten
        push    ds
        mov     ax,ds
        mov     es,ax
        mov     ax,0b800h
        mov     ds,ax
        mov     di,cursor_bg

        ; red/green page
        xor     bx,bx
        mov     bl,[cs:cursor_hbytes]
        mov     dl,[cs:cursor_rows]
        xor     cx,cx
.save_y_loop_rg:
        mov     cl,bl
        rep     movsb
        sub     si,bx
        ; handle scanline interleaving
        add     si,8192
        cmp     si,16384
        jb      .save_odd_rg
        sub     si,16304
.save_odd_rg:
        dec     dl
        jnz     .save_y_loop_rg

        ; blue/intensity page
        mov     si,[cs:cursor_ofs]
        add     si,16384
        xor     bx,bx
        mov     bl,[cs:cursor_hbytes]
        mov     dl,[cs:cursor_rows]
        xor     cx,cx
.save_y_loop_bi:
        mov     cl,bl
        rep     movsb
        sub     si,bx
        ; handle scanline interleaving
        add     si,8192
        cmp     si,32768
        jb      .save_odd_bi
        sub     si,16304
.save_odd_bi:
        dec     dl
        jnz     .save_y_loop_bi

        pop     ds

        ; draw cursor
        mov     ax,0b800h
        mov     es,ax
        mov     di,[cursor_ofs]
        mov     si,cursor_and
        ; calculate X-offset, load row count
        mov     cx,[cursor_x]
        and     cx,3
        shl     cl,1
        mov     ch,[cursor_rows]

.draw_y_loop:
        ; count horizontal bytes in bl
        xor     bx,bx

        ; handle first byte in line
        ; load one word of the inverted AND-mask for this line
        mov     ax,[si]
        xchg    al,ah
        shr     ax,cl
        ; restore non-inverted
        not     ax
        mov     al,ah
        ; apply the AND-mask
        and     al,[es:di]
        and     ah,[es:di+16384]
        mov     bp,ax
        mov     ax,[si+80]
        xchg    al,ah
        shr     ax,cl
        mov     al,ah
        ; apply the OR-mask
        or      ax,bp
        mov     [es:di+bx],al
        mov     [es:di+bx+16384],ah
        inc     bl

        ; handle rest of line
.draw_x_loop:
        cmp     bl,[cursor_hbytes]
        je      .draw_x_loop_end
        ; load one word of the inverted AND-mask for this line
        mov     ax,[si+bx-1]
        xchg    al,ah
        shr     ax,cl
        ; restore non-inverted
        not     ax
        mov     ah,al
        ; apply the AND-mask
        and     al,[es:di+bx]
        and     ah,[es:di+bx+16384]
        mov     bp,ax
        mov     ax,[si+bx+80-1]
        xchg    al,ah
        shr     ax,cl
        mov     ah,al
        ; apply the OR-mask
        or      ax,bp
        mov     [es:di+bx],al
        mov     [es:di+bx+16384],ah
        inc     bl
        jmp     .draw_x_loop
.draw_x_loop_end:
        add     si,5

        ; handle scanline interleaving
        add     di,8192
        cmp     di,16384
        jb      .draw_odd
        sub     di,16304
.draw_odd:
        dec     ch
        jnz     .draw_y_loop

        ret

;-------------- restore_background -------------------------------------
; Restore the screen content previously saved and overwritten by
; draw_cursor.
;
; Parameters:   --
; Returns:      --
;-----------------------------------------------------------------------
restore_background:
        mov     ax,0b800h
        mov     es,ax

        ; red/green page
        mov     di,[cursor_ofs]

        mov     si,cursor_bg
        xor     bx,bx
        mov     bl,[cursor_hbytes]
        mov     dl,[cursor_rows]
        xor     cx,cx
.y_loop_rg:
        mov     cl,bl
        rep     movsb
        sub     di,bx
        ; handle scanline interleaving
        add     di,8192
        cmp     di,16384
        jb      .odd_rg
        sub     di,16304
.odd_rg:
        dec     dl
        jnz     .y_loop_rg

        ; blue/intensity page
        mov     di,[cursor_ofs]
        add     di,16384

        xor     bx,bx
        mov     bl,[cursor_hbytes]
        mov     dl,[cursor_rows]
        xor     cx,cx
.y_loop_bi:
        mov     cl,bl
        rep     movsb
        sub     di,bx
        ; handle scanline interleaving
        add     di,8192
        cmp     di,32768
        jb      .odd_bi
        sub     di,16304
.odd_bi:
        dec     dl
        jnz     .y_loop_bi

        ret

;-------------- lock_cursor --------------------------------------------
; Locks the cursor in its current position without changing its
; visibility.
;
; Parameters:   --
; Returns:      --
; Notes:        Has to preserve all registers
;-----------------------------------------------------------------------
lock_cursor:
        ; hard synchronization
        pushf
        cli

        inc     word [cursor_lock]
        push    ax
        ; initialize new cursor position with current cursor position
        mov     ax,[cursor_x]
        mov     [cursor_new_x],ax
        mov     ax,[cursor_y]
        mov     [cursor_new_y],ax
        pop     ax

        popf

        ret

;-------------- unlock_cursor ------------------------------------------
; Unlocks the cursor and updates its position, if it has changed since
; the cursor has been locked.
;
; Parameters:   --
; Returns:      --
;-----------------------------------------------------------------------
unlock_cursor:
        ; hard synchronization
        pushf
        cli

        dec     word [cursor_lock]
        jnz     .end
        ; check if cursor should have moved and move for real
        mov     ax,[cursor_new_x]
        mov     bx,[cursor_new_y]
        cmp     ax,[cursor_x]
        jne     .move
        cmp     bx,[cursor_y]
        jne     .move
        jmp     .end
.move:  call    move_cursor

.end:   popf

        ret
		
;-------------- set_palette --------------------------------------------
; Sets the screen palette
;
; Parameters:   ax:si 	pointer to the palette data
; Returns:      --
;-----------------------------------------------------------------------
set_palette:
		push	ds
		push	ax
		push	bx
		push	cx
		push	dx
		push	es
		push	si
		push	di
		push	bp

		;int		29h
		add		si, 260
		
		; bp value will point to the intensity table
		mov		bp, si
		add		bp, 1024
		
		mov		ds, ax
		mov		dx, 256
		
		mov		ax, cs
		mov		es, ax
		mov		di, convert_palette
						
.palette_loop:
		xor		ah, ah
		
		lodsb		; Load flags
		;cmp		al, 1
		;je		.calculate_rgb_value
		;
		;; not processing this entry
		;inc		di
		;add		si, 3
		;jmp		.palette_loop_next

.calculate_rgb_value:
		lodsb		; Load red
		and		al, 0xe0
		or		ah, al
		
		lodsb		; Load green
		and		al, 0xc0
		mov		cl, 3
		shr		al, cl
		or		ah, al
		
		lodsb		; Load blue
		and		al, 0xe0
		mov		cl, 5
		shr		al, cl
		or		ah, al
		
		mov		al,	[ds:bp]		; Load intensity value (0-100 range)
		add		bp, 2
		mov		bx, intensity_mask
		cs      xlatb			; Load intensity mask into AL
		
		and		al, ah			; Apply RGB value with intensity mask
				
		xor		ah, ah			; Convert to look up index into array
		shl		ax, 1

		mov		bx, ax
		mov		ax, [cs:convert_323_palette + bx]
		;mov		bx, convert_323_palette
		;cs      xlatb	; Convert RGB value to CGA pattern
		
		stosb		; Store pattern in palette look up table
		add		di, 255
		xchg	al, ah
		stosb
		sub		di, 256

.palette_loop_next:
		dec		dx
		jnz		.palette_loop
		
		; update the screen if we have the framebuffer segment stored
		mov		si,	[framebuffer_segment_cache]
		cmp		si, 0
		jz 		.finish_palette_update
		
		xor		ax, ax
		xor		bx, bx
		mov		cx, 200
		mov		dx, 320
		;call	update_rect
		
.finish_palette_update:
		pop		bp
		pop		di
		pop		si
		pop		es
		pop		dx
		pop		cx
		pop		bx
		pop		ax
		pop		ds

		ret