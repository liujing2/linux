/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _DRIVERS_VIRTIO_VIS_H
#define _DRIVERS_VIRTIO_VIS_H

#include <linux/device.h>

int vis_irq_vector(struct device *dev, unsigned int nr);

#endif
