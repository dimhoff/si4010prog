/**
 * c2_hw_c2drv.c - SiLabs C2 master bus interface using Linux LPT port driver 
 *
 * Based on code written by Wojciech M. Zabolotny (wzab@ise.pw.edu.pl)
 * and is published under the GPL (Gnu Public License) version 2.0 or later
 */
#include "c2_hw.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#include "c2_ioctls.h"

struct c2_hw_data {
	int fd;
};

c2_hw_t *c2_hw_create(const char *path)
{
	c2_hw_t *hw = (c2_hw_t *) malloc(sizeof(c2_hw_t));
	hw->fd = open(path, O_RDWR);
	if (hw->fd < 0) {
		free(hw);
		return NULL;
	}
	return hw;
}

void c2_hw_reset(c2_hw_t *hw)
{
	ioctl(hw->fd, C2_IOCRESET, NULL);
	usleep(2);
}

void c2_hw_qreset(c2_hw_t *hw)
{
	ioctl(hw->fd, C2_IOCQRESET, NULL);    // stop execution on the MCU
}

void c2_hw_read_addr(c2_hw_t *hw, unsigned char *addr)
{
	ioctl(hw->fd, C2_IOCAREAD, &addr);
}

void c2_hw_write_addr(c2_hw_t *hw, unsigned char addr)
{
	ioctl(hw->fd, C2_IOCAWRITE, &addr);
}

void c2_hw_read_data(c2_hw_t *hw, unsigned char *data, size_t len)
{
	unsigned long v = 0;

	assert(len == 1);
	ioctl(hw->fd, C2_IOCD1READ, &v);
	*data = v;
}

void c2_hw_write_data(c2_hw_t *hw, const unsigned char *data, size_t len)
{
	assert(len == 1);
	ioctl(hw->fd, C2_IOCD1WRITE, data);
}

void c2_hw_destroy(c2_hw_t *hw)
{
	if (hw == NULL) return;

	if (hw->fd != -1) {
		close(hw->fd);
	}
	free(hw);
}
