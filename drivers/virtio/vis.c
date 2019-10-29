// SPDX-License-Identifier: GPL-2.0

#include <linux/msi.h>
#include <linux/of_irq.h>
#include "vis.h"

/**
 * vis_irq_vector - return Linux IRQ number of a device vector
 * @dev: device to operate on
 * @nr: device-relative interrupt vector index (0-based).
 */
int vis_irq_vector(struct device *dev, unsigned int nr)
{
	struct msi_desc *entry;
	int i = 0;

	for_each_msi_entry(entry, dev) {
		if (i == nr)
			return entry->irq;
		i++;
        }

	WARN_ON_ONCE(1);
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(vis_irq_vector);


/**
 * vis_setup_irqs - setup irqs from VIS domain
 * @dev: device to operate on
 * @nvec: request interrupt vector number.
 */
int vis_setup_irqs(struct device *dev, int nvec)
{
	struct irq_domain *vis_domain = vis_get_irq_domain();
	struct irq_alloc_info info;

	if (!vis_domain)
		return -ENOSYS;

	init_irq_alloc_info(&info, NULL);
	info.type = X86_IRQ_ALLOC_TYPE_VIS;

	return msi_domain_alloc_irqs(vis_domain, dev, nvec);
}
EXPORT_SYMBOL_GPL(vis_setup_irqs);

static void vis_teardown_irqs(struct device *dev)
{
	struct irq_domain *domain;

	domain = dev_get_msi_domain(dev);
	if (domain && irq_domain_is_hierarchy(domain))
		/* VIS domain uses hierarchy */
		msi_domain_free_irqs(domain, dev);
	else
		// TODO
		return;
}

void free_vis_irqs(struct device *dev)
{
	struct list_head *msi_list = dev_to_msi_list(dev);
	struct msi_desc *tmp, *entry;
	int i;

	for_each_msi_entry(entry, dev) {
		if (entry->irq)
			for (i = 0; i < entry->nvec_used; i++)
				BUG_ON(irq_has_action(entry->irq + i));
	}
	vis_teardown_irqs(dev);

	list_for_each_entry_safe(entry, tmp, msi_list, list) {
		/* TODO: Do we need iounmap for table */
		list_del(&entry->list);
		free_msi_entry(entry);
	}
}
EXPORT_SYMBOL_GPL(free_vis_irqs);


/**
 * vis_domain_calc_hwirq - Generate a unique ID for a VIS interrupt
 * @desc:	Pointer to the msi descriptor
 *
 * The ID number is only used within the irqdomain.
 */
irq_hw_number_t vis_domain_calc_hwirq(struct msi_desc *desc)
{
	static irq_hw_number_t num = 0;

	return num++;
}
/**
 * TODO
 * Write to the MBA register for this desc,
 * desc->vis.vis_attrib.entry_nr shows the MBA bit offset
 */
static u32 __vis_msix_desc_mask_irq(struct msi_desc *desc, u32 flag)
{
	/* Get all vectors mask bits */
	u32 mask_bits = desc->vis.masked;

	mask_bits &= ~(1 << desc->vis.vis_attrib.entry_nr);

	if (flag) {
		mask_bits |= (1 << desc->vis.vis_attrib.entry_nr);
	}
	writel(mask_bits, desc->vis.base.mask_base);

	return mask_bits;
}

static void vis_set_mask_bit(struct irq_data *data, u32 flag)
{
        struct msi_desc *desc = irq_data_get_msi_desc(data);

        desc->vis.masked = __vis_msix_desc_mask_irq(desc, flag);
}

/**
 * vis_mask_irq - Generic irq chip callback to mask interrupts
 * @data:       pointer to irqdata associated to that interrupt
 */
void vis_mask_irq(struct irq_data *data)
{
        vis_set_mask_bit(data, 1);
}
EXPORT_SYMBOL_GPL(vis_mask_irq);

/**
 * vis_unmask_irq - Generic irq chip callback to unmask VIS interrupts
 * @data:       pointer to irqdata associated to that interrupt
 */
void vis_unmask_irq(struct irq_data *data)
{
        vis_set_mask_bit(data, 0);
}
EXPORT_SYMBOL_GPL(vis_unmask_irq);

/**
 * vis_domain_write_msg - Helper to write VIS message to device config space
 * @irq_data:   Pointer to interrupt data of the VIS interrupt
 * @msg:        Pointer to the message
 */
void vis_domain_write_msg(struct irq_data *irq_data, struct msi_msg *msg)
{
	struct msi_desc *desc = irq_data_get_msi_desc(irq_data);
	void __iomem *base = desc->vis.base.vet_base + desc->vis.vis_attrib.entry_nr * 12;

	writel(msg->address_lo, base);
	writel(msg->address_hi, base + 4);
	writel(msg->data, base + 8);

	desc->msg = *msg;
}
EXPORT_SYMBOL_GPL(vis_domain_write_msg);
