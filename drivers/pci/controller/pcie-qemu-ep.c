// SPDX-License-Identifier: GPL-2.0

/*
 *
 *
 */

#include "asm-generic/pci_iomap.h"
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-epc.h>
#include <asm/io.h>

/*
 *
 */

enum {
    QEMU_EPC_BAR_CTRL = 0,
    QEMU_EPC_BAR_PCI_CFG = 1,
    QEMU_EPC_BAR_BAR_CFG = 2,
    QEMU_EPC_BAR_WINDOW = 3,
};

enum {
	QEMU_EP_BAR_CFG_OFF_MASK = 0x00,
	QEMU_EP_BAR_CFG_OFF_NUMBER = 0x01,
	QEMU_EP_BAR_CFG_OFF_FLAGS = 0x02,
	QEMU_EP_BAR_CFG_OFF_RSV = 0x04,
	QEMU_EP_BAR_CFG_OFF_PHYS_ADDR = 0x08,
	QEMU_EP_BAR_CFG_OFF_SIZE = 0x10,

	QEMU_EP_BAR_CFG_SIZE = 0x18
};

enum {
    QEMU_EP_CTRL_OFF_START = 0x00,
    QEMU_EP_CTRL_OFF_WIN_START = 0x8,
    QEMU_EP_CTRL_OFF_WIN_SIZE = 0x10,
    QEMU_EP_CTRL_OFF_IRQ_TYPE = 0x18, 
    QEMU_EP_CTRL_OFF_IRQ_NUM = 0x1c,
    QEMU_EP_CTRL_OFF_OB_MAP_MASK = 0x20,
    QEMU_EP_CTRL_OFF_OB_MAP_BASE = 0x28,
};

struct qemu_ep {
	void __iomem *cfg_base;
	void __iomem *bar_base;
	void __iomem *ctl_base;
};

#define QEMU_EP_DRV_NAME "QEMU PCIe EP driver"
#define QEMU_EPC_VERSION 0x00

static inline u8 qemu_ep_cfg_read8(struct qemu_ep *qep, unsigned offset)
{
    return ioread8(qep->cfg_base + offset);
}

static inline void qemu_ep_cfg_write8(struct qemu_ep *qep, unsigned offset, u8 value)
{
    iowrite8(value, qep->cfg_base + offset);
}

static inline void qemu_ep_cfg_write16(struct qemu_ep *qep, unsigned offset, u16 value)
{
    iowrite16(value, qep->cfg_base + offset);
}

static inline void  qemu_ep_cfg_write32(struct qemu_ep *qep, unsigned offset, u32 value)
{
    iowrite32(value, qep->cfg_base + offset);
}

static inline void qemu_ep_cfg_write64(struct qemu_ep *qep, unsigned offset, u64 value)
{
//#if CONFIG_64BIT
//    iowrite64(value, qep->cfg_base + offset);
//#else
    iowrite32(value, qep->cfg_base + offset);
    iowrite32(value >> 32, qep->cfg_base + offset + 4);
//#endif
}

static inline uint8_t qemu_ep_bar_cfg_read8(struct qemu_ep *qep, unsigned offset)
{
	return ioread8(qep->bar_base + offset);
}

static inline void qemu_ep_bar_cfg_write8(struct qemu_ep *qep, unsigned offset, uint8_t value)
{
	iowrite8(value, qep->bar_base + offset);
}

static inline void qemu_ep_bar_cfg_write32(struct qemu_ep *qep, unsigned offset, uint32_t value)
{
	iowrite32(value, qep->bar_base + offset);
}

static inline void qemu_ep_bar_cfg_write64(struct qemu_ep *qep, unsigned offset, uint64_t value)
{
	qemu_ep_bar_cfg_write32(qep, offset, (uint32_t)value);
	qemu_ep_bar_cfg_write32(qep, offset + 4, value >> 32);
}

static inline void qemu_ep_ctl_write8(struct qemu_ep *qep, unsigned offset, uint8_t value)
{
	iowrite8(value, qep->ctl_base + offset);
}

static inline void qemu_ep_ctl_write32(struct qemu_ep *qep, unsigned offset, uint32_t value)
{
	iowrite32(value, qep->ctl_base + offset);
}

static inline void qemu_ep_ctl_write64(struct qemu_ep *qep, unsigned offset, uint64_t value)
{
	qemu_ep_ctl_write32(qep, offset, (uint32_t)value);
	qemu_ep_ctl_write32(qep, offset + 4, value >> 32);
}

static inline uint32_t qemu_ep_ctl_read32(struct qemu_ep *qep, unsigned offset)
{
	return ioread32(qep->ctl_base + offset);
}

static inline uint64_t qemu_ep_ctl_read64(struct qemu_ep *qep, unsigned offset)
{
	return (uint64_t)qemu_ep_ctl_read32(qep, offset + 4) << 32 | qemu_ep_ctl_read32(qep, offset);
}

static int qemu_ep_write_header(struct pci_epc *epc, u8 fn, u8 vfn,
				struct pci_epf_header *hdr)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);

	pr_info("%s: vendor 0x%x, device 0x%x\n", __func__, hdr->vendorid, hdr->deviceid);

  qemu_ep_cfg_write16(qep, PCI_VENDOR_ID, hdr->vendorid);
  qemu_ep_cfg_write16(qep, PCI_DEVICE_ID, hdr->deviceid);
  qemu_ep_cfg_write8(qep, PCI_REVISION_ID, hdr->revid);
  qemu_ep_cfg_write8(qep, PCI_CLASS_PROG, hdr->progif_code);
  qemu_ep_cfg_write8(qep, PCI_CLASS_DEVICE, hdr->baseclass_code);
  qemu_ep_cfg_write8(qep, PCI_CLASS_DEVICE + 1, hdr->subclass_code);
  qemu_ep_cfg_write8(qep, PCI_CACHE_LINE_SIZE, hdr->cache_line_size);
  qemu_ep_cfg_write8(qep, PCI_SUBSYSTEM_VENDOR_ID, hdr->subsys_vendor_id);
  qemu_ep_cfg_write8(qep, PCI_SUBSYSTEM_ID, hdr->subsys_id);
  qemu_ep_cfg_write8(qep, PCI_INTERRUPT_PIN, hdr->interrupt_pin);

	return 0;
}

static int qemu_ep_set_bar(struct pci_epc *epc, u8 fn, u8 vfn,
			   struct pci_epf_bar *bar)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);
  u8 mask;

  pr_info("%s:%d bar %d\n", __func__, __LINE__, bar->barno);

	qemu_ep_bar_cfg_write8(qep, QEMU_EP_BAR_CFG_OFF_NUMBER, bar->barno);
	qemu_ep_bar_cfg_write64(qep, QEMU_EP_BAR_CFG_OFF_PHYS_ADDR, bar->phys_addr);
  qemu_ep_bar_cfg_write64(qep, QEMU_EP_BAR_CFG_OFF_SIZE, bar->size);
  qemu_ep_bar_cfg_write8(qep, QEMU_EP_BAR_CFG_OFF_FLAGS, bar->flags);

  mask = qemu_ep_bar_cfg_read8(qep, QEMU_EP_BAR_CFG_OFF_MASK) | BIT(bar->barno);
  qemu_ep_bar_cfg_write8(qep, QEMU_EP_BAR_CFG_OFF_MASK, mask);

	return 0;
}

static void qemu_ep_clear_bar(struct pci_epc *epc, u8 fn, u8 vfn,
			      struct pci_epf_bar *bar)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);
	uint8_t mask;

	mask = qemu_ep_bar_cfg_read8(qep, QEMU_EP_BAR_CFG_OFF_MASK) & ~BIT(bar->barno);
  qemu_ep_bar_cfg_write8(qep, QEMU_EP_BAR_CFG_OFF_MASK, mask);
}

static int qemu_ep_map_addr(struct pci_epc *epc, u8 fn, u8 vfn,
			    phys_addr_t addr, u64 pci_addr, size_t size)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);
	uint64_t mask, tmp;
	unsigned idx = 0;
	size_t offset;

	mask = qemu_ep_ctl_read64(qep, QEMU_EP_CTRL_OFF_OB_MAP_MASK);
	tmp = mask;
	while(tmp) {
		if (tmp & 0) {
			break;
		}

		idx++;
		tmp >>= 1;
	}

	offset = QEMU_EP_CTRL_OFF_OB_MAP_BASE + 0x20 * idx;
	qemu_ep_ctl_write64(qep, offset, addr);
	qemu_ep_ctl_write64(qep, offset + 0x8, pci_addr);
	qemu_ep_ctl_write64(qep, offset + 0x10, size);

	qemu_ep_ctl_write64(qep, QEMU_EP_CTRL_OFF_OB_MAP_MASK, mask | BIT(idx));

	pr_info("%s: [%d] phys 0x%llx pci 0x%llx size 0x%lx\n", __func__, idx, addr, pci_addr, size);

	return 0;
}

static void qemu_ep_unmap_addr(struct pci_epc *epc, u8 fn, u8 vfn,
			       phys_addr_t addr)
{
	uint64_t mask;
	uint64_t phys;
	struct qemu_ep *qep = epc_get_drvdata(epc);

	mask = qemu_ep_ctl_read64(qep, QEMU_EP_CTRL_OFF_OB_MAP_MASK);

	for(int i = 0; i<16; i++) {

		phys = qemu_ep_ctl_read64(qep, QEMU_EP_CTRL_OFF_OB_MAP_BASE + i * 0x20);
		if (phys == addr) {
			mask &= ~BIT(i);
			qemu_ep_ctl_write64(qep, QEMU_EP_CTRL_OFF_OB_MAP_MASK, mask);
		}
	}

	pr_info("%s addr 0x%llx\n", __func__, addr);
}

static int qemu_ep_raise_irq(struct pci_epc *epc, u8 fn, u8 vfn,
			     enum pci_epc_irq_type type, u16 interrupt_num)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);
	pr_info("%s type %d num %d\n", __func__, type, interrupt_num);

	qemu_ep_ctl_write32(qep, QEMU_EP_CTRL_OFF_IRQ_TYPE, type);
	qemu_ep_ctl_write32(qep, QEMU_EP_CTRL_OFF_IRQ_NUM, interrupt_num);

	return 0;
}

static int qemu_ep_start(struct pci_epc *epc)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);

	pr_info("%s\n", __func__);

	qemu_ep_ctl_write8(qep, 0, 1);

	return 0;
}

static const struct pci_epc_features qemu_epc_features = {
	.linkup_notifier = false,
	.core_init_notifier = false,
	.msi_capable = false,
	.msix_capable = false,
};

static const struct pci_epc_features *qemu_ep_get_features(struct pci_epc *epc,
							   u8 fn, u8 vfn)
{
	return &qemu_epc_features;
}

static const struct pci_epc_ops qemu_epc_ops = {
	.write_header = qemu_ep_write_header,
	.set_bar = qemu_ep_set_bar,
	.clear_bar = qemu_ep_clear_bar,
	.map_addr = qemu_ep_map_addr,
	.unmap_addr = qemu_ep_unmap_addr,
	//     .set_msi = ,
	//     .get_msi = ,
	.raise_irq = qemu_ep_raise_irq,
	.start = qemu_ep_start,
	.get_features = qemu_ep_get_features,
};

static int qemu_ep_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct device *dev = &pdev->dev;
	struct pci_epc *epc;
	struct qemu_ep *qep;
	int err;

	dev_info(dev, "probe is called\n");

	if (pdev->revision != QEMU_EPC_VERSION) {
		dev_err(dev, "Driver supports version 0x%x, but device is 0x%x\n", QEMU_EPC_VERSION, pdev->revision);
		return -ENOTSUPP;
	}

	qep = devm_kzalloc(dev, sizeof(*qep), GFP_KERNEL);
	if (!qep) {
		dev_err(dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	epc = devm_pci_epc_create(dev, &qemu_epc_ops);
	if (IS_ERR(epc)) {
		dev_err(dev, "Failed to create epc device\n");
		err = PTR_ERR(epc);
		goto err_qep_kfree;
	}

	epc_set_drvdata(epc, qep);

	epc->max_functions = 1;

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(dev, "Cannot enable PCI device\n");
		goto err_release_epc;
	}

	err = pci_request_regions(pdev, QEMU_EP_DRV_NAME);
	if (err) {
		dev_err(dev, "Cannot obtain PCI resources\n");
		goto err_disable_pdev;
	}

	err = dma_set_mask(dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(dev, "No usable DMA configuration\n");
		goto err_disable_pdev;
	}

	qep->cfg_base = pci_iomap(pdev, QEMU_EPC_BAR_PCI_CFG, PCI_CFG_SPACE_EXP_SIZE);
	if (!qep->cfg_base) {
		dev_err(dev, "Cannot map device registers\n");
		err = -ENOMEM;
		goto err_disable_pdev;
	}

	qep->bar_base = pci_iomap(pdev, QEMU_EPC_BAR_BAR_CFG, QEMU_EP_BAR_CFG_SIZE);
	if (!qep->bar_base) {
		dev_err(dev, "Cannot map device register for bar\n");
		err = -ENOMEM;
		goto err_unmap_cfg;
	}

	qep->ctl_base = pci_iomap(pdev, QEMU_EPC_BAR_CTRL, 64);
	if (!qep->bar_base) {
		dev_err(dev, "Cannot map ctrl register\n");
		err = -ENOMEM;
		goto err_unmap_bar;
	}

	{
		phys_addr_t phys = qemu_ep_ctl_read64(qep, QEMU_EP_CTRL_OFF_WIN_START);
		uint64_t size = qemu_ep_ctl_read64(qep, QEMU_EP_CTRL_OFF_WIN_SIZE);
		dev_info(dev, "window phys 0x%llx, size 0x%llx\n", phys, size);

		err = pci_epc_mem_init(epc, phys, size, PAGE_SIZE);
		if (err < 0) {
			dev_err(dev, "oh no\n");
			goto err_release_epc;
		}
	}

	pci_set_master(pdev);

	return 0;

err_unmap_bar:
	pci_iounmap(pdev, qep->bar_base);
err_unmap_cfg:
	pci_iounmap(pdev, qep->cfg_base);
err_disable_pdev:
	pci_disable_device(pdev);
err_release_epc:
	devm_pci_epc_destroy(dev, epc);
err_qep_kfree:
	devm_kfree(dev, qep);

	return err;
}

#define PCI_DEVICE_ID_REDHAT_PCI_EP 0x0013

static const struct pci_device_id qemu_ep_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_DEVICE_ID_REDHAT_PCI_EP) },
	/* required last entry */
	{
		0,
	}
};
// TODO: MODULE_DEVICE_TABLE(pci, qemu_ep_id_table)

static struct pci_driver qemu_ep_driver = {
	.name = QEMU_EP_DRV_NAME,
	.id_table = qemu_ep_id_table,
	.probe = qemu_ep_probe,
	//     .remove = qemu_ep_remove,
};

static int __init pcie_qemu_ep_init(void)
{
	pr_info("QEMU PCI Endpoint controller is prepared\n");
	return pci_register_driver(&qemu_ep_driver);
}
module_init(pcie_qemu_ep_init);

static void __exit pcie_qemu_ep_exit(void)
{
	pci_unregister_driver(&qemu_ep_driver);
}
module_exit(pcie_qemu_ep_exit);

