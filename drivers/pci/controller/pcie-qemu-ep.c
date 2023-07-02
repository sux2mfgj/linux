// SPDX-License-Identifier: GPL-2.0

/*
 *
 *
 */

#include <linux/module.h>
#include <linux/pci.h>

static int qemu_ep_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    struct device *dev = &pdev->dev;

    return 0;
}

#define PCI_DEVICE_ID_REDHAT_PCI_EP 0x0013

static const struct pci_device_id qemu_ep_id_table[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_DEVICE_ID_REDHAT_PCI_EP) },
    /* required last entry */
    {0,}
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
    return pci_register_driver(&qemu_ep_driver);
}
module_init(pcie_qemu_ep_init);

static void __exit pcie_qemu_ep_exit(void)
{
}
module_exit(pcie_qemu_ep_exit);

