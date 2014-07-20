/**
 * c2_hw.h - C2 Master bus interface functions
 *
 * Copyright (c) 2014, David Imhoff <dimhoff_devel@xs4all.nl>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __C2_HW_H__
#define __C2_HW_H__

#include <stdlib.h>

typedef struct c2_hw_data c2_hw_t;

c2_hw_t *c2_hw_create(const char *path);
int c2_hw_reset(c2_hw_t *hw);
int c2_hw_qreset(c2_hw_t *hw);
int c2_hw_read_addr(c2_hw_t *hw, unsigned char *addr);
int c2_hw_write_addr(c2_hw_t *hw, unsigned char addr);
int c2_hw_data_read(c2_hw_t *hw, unsigned char *data, size_t len);
int c2_hw_data_write(c2_hw_t *hw, const unsigned char *data, size_t len);
void c2_hw_destroy(c2_hw_t *hw);

#endif // __C2_HW_H__
