;;
; Si4010 Get Tuned PA Capacitor bank value
;
; This program runs the Si4010 auto antenna matching code to tune the antenna
; and returns the capacitor bank setting(wNominalCap).
; See 'tune_cap_bank.py' for usage information.
;
; Target: Si4010
; Toolchain: SDCC
;
; Copyright (c) 2014, David Imhoff <dimhoff.devel@gmail.com>
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;     * Redistributions of source code must retain the above copyright
;       notice, this list of conditions and the following disclaimer.
;     * Redistributions in binary form must reproduce the above copyright
;       notice, this list of conditions and the following disclaimer in the
;       documentation and/or other materials provided with the distribution.
;     * Neither the name of the author nor the names of its contributors may
;       be used to endorse or promote products derived from this software
;       without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;

vSys_Setup = 0x80B1      ; void vSys_Setup(BYTE biFirstBatteryInsertWaitTime)
vSys_BandGapLdo = 0x80A2 ; void vSys_BandGapLdo(BYTE biBandGapLdoCtrl)

vPa_Setup = 0x11E9       ; void vPa_Setup(tPa_Setup xdata *priSetup)
vPa_Tune = 0x8090        ; void vPa_Tune(WORD wiTemp)

vFCast_Setup = 0x80F9    ; void vFCast_Setup(void)
vFCast_Tune = 0x80F6     ; void vFCast_Setup(void)

iDmdTs_GetLatestTemp = 0x802A   ; int iDmdTs_GetLatestTemp(void)
bDmdTs_GetSamplesTaken = 0x8036 ; BYTE bDmdTs_GetSamplesTaken(void)

vDmdTs_ClearDmdIntFlag = 0x8030 ; void vDmdTs_ClearDmdIntFlag(void)
vDmdTs_IsrCall = 0x11EC ; void vDmdTs_IsrCall(void)

PDMD = 0xb8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Jump Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.area JUMP_TABLE (ABS)
	.org	0
	ljmp	init
	.org	0x13
	ljmp	isr_dmd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Parameters writen from debugger
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.area	PARAM (ABS)
	.org	0x40
; Frequency (float)
freq::
	.blkb	4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Parameters writen from debugger (XDATA)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.area _XDATA
; PA setup config (tPa_Setup)
pa_config::
	.blkb	12

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Code Area
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.area _CODE
init::
	; setup stack pointer
	mov	SP, #0x80

main:
	; Set DMD interrupt to high priority,
	; any other interrupts have to stay low priority
	setb	PDMD

	; Initialize Si4010 API
	; wait time is 0 since bandgap never turned off
	mov	R7, #0
	lcall	vSys_Setup

	; Setup the bandgap for working with the temperature sensor here.
	mov	R7, #1
	lcall	vSys_BandGapLdo

	; Required bPA_TRIM clearing before calling vPa_Setup()
	mov	R6, #>pa_config
	mov	R7, #pa_config
	lcall	vPa_Setup

	; Setup and run the frequency casting.
	lcall	vFCast_Setup

	; Setup and run the frequency casting.
	mov	R4, freq
	mov	R5, freq+1
	mov	R6, freq+2
	mov	R7, freq+3
	lcall	vFCast_Tune

	; Wait for a temperature sample and adjust oscilator with it
wait_samples:
	lcall	bDmdTs_GetSamplesTaken
	mov	A, R7
	jz	wait_samples

	; Tune PA
	; This tunes the PA to the antenna by changing wPA_CAP
	lcall	iDmdTs_GetLatestTemp
	lcall	vPa_Tune

	; Wait for programmer, value of tuned capacitor bank is now available in wPA_CAP(0x400c)
loop_breakpoint::
	sjmp	.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Interrup service routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
isr_dmd:
	push    ACC
	push    B
	push    DPH
	push    DPL
	push    PSW
	mov     PSW,#0x10
	lcall   vDmdTs_ClearDmdIntFlag
	lcall   vDmdTs_IsrCall
	pop     PSW
	pop     DPL
	pop     DPH
	pop     B
	pop     ACC
	reti
