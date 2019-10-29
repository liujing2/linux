/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _DRIVERS_VIRTIO_VIS_H
#define _DRIVERS_VIRTIO_VIS_H

#include <linux/device.h>

int vis_irq_vector(struct device *dev, unsigned int nr);
int vis_setup_irqs(struct device *dev, int nvec);
void free_vis_irqs(struct device *dev);
#endif
