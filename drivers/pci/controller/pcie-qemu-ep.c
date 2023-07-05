// SPDX-License-Identifier: GPL-2.0

/*
 *
 *
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-epc.h>

struct qemu_ep {
	void __iomem *cfg_base;
};

#define QEMU_EP_DRV_NAME "QEMU PCIe EP driver"

static int qemu_ep_write_header(struct pci_epc *epc, u8 fn, u8 vfn,
				struct pci_epf_header *hdr)
{
	struct qemu_ep *qep = epc_get_drvdata(epc);

	memcpy_toio(qep->cfg_base, hdr, sizeof(*hdr));

	return 0;
}

static int qemu_ep_set_bar(struct pci_epc *epc, u8 fn, u8 vfn,
			   struct pci_epf_bar *bar)
{
	return -ENOTSUPP;
}

static void qemu_ep_clear_bar(struct pci_epc *epc, u8 fn, u8 vfn,
			      struct pci_epf_bar *bar)
{
}

static int qemu_ep_map_addr(struct pci_epc *epc, u8 fn, u8 vfn,
			    phys_addr_t addr, u64 pci_addr, size_t size)
{
	return -ENOTSUPP;
}

static void qemu_ep_unmap_addr(struct pci_epc *epc, u8 fn, u8 vfn,
			       phys_addr_t addr)
{
}

static int qemu_ep_raise_irq(struct pci_epc *epc, u8 fn, u8 vfn,
			     enum pci_epc_irq_type type, u16 interrupt_num)
{
	return -ENOTSUPP;
}

static int qemu_ep_start(struct pci_epc *epc)
{
	return -ENOTSUPP;
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

	// TODO above valus should be got from qemu-ep.
	epc->max_functions = 1;

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(dev, "Cannot enable PCI device\n");
		goto err_release_epc;
	}

	if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
		dev_err(dev, "Cannot find proper PCI BAR\n");
		err = -ENODEV;
		goto err_disable_pdev;
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

	qep->cfg_base = pci_iomap(pdev, 0, PCI_CFG_SPACE_SIZE);
	if (!qep->cfg_base) {
		dev_err(dev, "Cannot map device registers\n");
		err = -ENOMEM;
		goto err_disable_pdev;
	}

	pci_set_master(pdev);

	return 0;

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

