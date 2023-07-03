// SPDX-License-Identifier: GPL-2.0

/*
 *
 *
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-epc.h>

static int qemu_ep_write_header(struct pci_epc *epc, u8 fn, u8 vfn,
				struct pci_epf_header *hdr)
{
	return -ENOTSUPP;
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

	dev_info(dev, "probe is called\n");

	epc = devm_pci_epc_create(dev, &qemu_epc_ops);
	if (IS_ERR(epc)) {
		dev_err(dev, "failed to create epc device\n");
		return PTR_ERR(epc);
	}

	// TODO above valus should be got from qemu-ep.
	epc->max_functions = 1;

	return 0;
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
	.name = "QEMU PCIe EP driver",
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

