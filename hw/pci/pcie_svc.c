/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * PCIe Streamlined Virtual Channel (SVC) Extended Capability
 *
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 */

#include "qemu/osdep.h"
#include "qemu/bitops.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pcie.h"
#include "hw/pci/pcie_svc.h"
#include "hw/pci/pcie_port.h"

static void pcie_svc_update_map(PCIDevice *dev)
{
    uint32_t non_uio_ctrl = SVC_VC0_PROTOCOL | SVC_VC_ENABLE;
    uint32_t uio_ctrl = SVC_UIO_PROTOCOL_SELECTED | SVC_VC_ENABLE;
    int offset;

    if (!pci_is_express(dev) || !dev->exp.svc_cap) {
        return;
    }

    offset = pcie_find_capability(dev, PCI_EXT_CAP_ID_SVC);

    /* SVC VC0 & VC3 initialization */
    pci_set_long(dev->config + offset + SVC_RES_CTRL(0), BIT(0) | non_uio_ctrl);
    pci_set_long(dev->config + offset + SVC_RES_CTRL(3), BIT(3) | uio_ctrl);

    /* SVC VC4 initialization - optional */
    if (dev->exp.svc.uio_opt_svc) {
        pci_set_long(dev->config + offset + SVC_RES_CTRL(4), BIT(4) | uio_ctrl);
    }
}

int pcie_config_uio_svc(PCIDevice *d, Error **errp)
{
    PCIEPort *p = PCIE_PORT(d);

    if (!get_uio_mandatory_svc(p)
        || pcie_svc_cap_init(d, PCI_EXT_CAP_BASE_OFFSET, errp) < 0) {
        return -1;
    }

    if (get_uio_optional_svc(p)) {
        pcie_svc_set_vc4(d, true);
    }

    return 0;
}

int pcie_svc_cap_init(PCIDevice *dev, uint16_t offset, Error **errp)
{
    uint32_t hdr;

    if (!pci_is_express(dev)) {
        error_setg(errp, "SVC ECAP requires PCIe");
        return -EINVAL;
    }

    /*
     * If no other ECAPs are present, make SVC the first at 0x100.
     * This avoids pcie_add_capability() asserting on a non-0x100 offset.
     */
    hdr = pci_get_long(dev->config + PCI_CONFIG_SPACE_SIZE);
    if (hdr == 0) {
        offset = PCI_CONFIG_SPACE_SIZE;
    }

    pcie_add_capability(dev, PCI_EXT_CAP_ID_SVC, 1, offset,
                        PCI_EXT_CAP_SVC_SIZE);
    dev->exp.svc_cap = offset;
    dev->exp.svc.ctrl = 0;
    dev->exp.svc.status = 0;
    dev->exp.svc.uio_mand_svc = true;
    dev->exp.svc.uio_opt_svc = false;

    pci_set_long(dev->config + offset + PCIE_SVC_CAP_HEAD_OFFSET,
                 (NEXT_CAP_OFF | PCIE_SVC_CAP_VER | PCI_EXT_CAP_ID_SVC));
    pci_set_long(dev->wmask + offset + PCIE_SVC_CTL_OFFSET,
                 PCIE_SVC_CTL_ENABLE);
    pci_set_long(dev->config + offset + PCIE_SVC_CAP_OFFSET,
                 PCIE_SVC_CAP1_EVCC);

    for (int i = 0; i <= PCIE_SVC_CAP1_EVCC; i++) {
        uint32_t res_cap_value = (1U << 8) | SVC_VC_ID(i);
        uint32_t res_ctrl_value = SVC_TC_VC_MAP_ENABLE
                                  | SVC_VC_PROTOCOL_SELECTED
                                  | SHARED_FLOW_CONTROL_USAGE_LIMIT_ENABLE
                                  | SHARED_FLOW_CONTROL_USAGE_LIMIT
                                  | SVC_VC_ENABLE;

        if (i == 0) {
            res_cap_value = SVC_VC0_PROTOCOL | SVC_VC_ID(i);
        } else if (i == 3) {
            res_cap_value = SVC_VC3_PROTOCOL | SVC_VC_ID(i);
        } else if (i == 4) {
            res_cap_value = SVC_VC4_PROTOCOL | SVC_VC_ID(i);
        }

        pci_set_long(dev->config + offset + SVC_RES_CAP(i), res_cap_value);
        pci_set_long(dev->wmask + offset + SVC_RES_CTRL(i), res_ctrl_value);
        pci_set_long(dev->config + offset + SVC_RES_STATUS(i), 0);
    }
    pcie_svc_update_map(dev);
    return 0;
}

void pcie_svc_cap_reset(PCIDevice *dev)
{
    uint32_t offset;

    if (!pci_is_express(dev) || !dev->exp.svc_cap) {
        return;
    }

    offset = dev->exp.svc_cap;
    dev->exp.svc.ctrl = 0;
    dev->exp.svc.status = 0;
    pcie_svc_update_map(dev);
    pci_set_long(dev->config + offset + PCIE_SVC_CTL_OFFSET, 0);
    pci_set_long(dev->config + offset + PCIE_SVC_STA_OFFSET, 0);
}

void pcie_svc_set_vc4(PCIDevice *dev, bool enable)
{
    if (!pci_is_express(dev) || !dev->exp.svc_cap) {
        return;
    }

    dev->exp.svc.uio_opt_svc = enable;
    pcie_svc_update_map(dev);
}

static void pcie_svc_apply_gating(PCIDevice *dev)
{
    uint16_t offset = dev->exp.svc_cap;
    uint32_t ctrl, status;

    if (!offset) {
        return;
    }

    ctrl = pci_get_long(dev->config + offset + PCIE_SVC_CTL_OFFSET);
    status = pci_get_long(dev->config + offset + PCIE_SVC_STA_OFFSET);
    dev->exp.svc.ctrl = ctrl;
    dev->exp.svc.status = status;
}

void pcie_svc_cap_write_config(PCIDevice *dev,
                               uint32_t addr, uint32_t val, int len)
{
    uint16_t offset = dev->exp.svc_cap;

    if (!offset) {
        return;
    }

    if (ranges_overlap(addr, len, offset + PCIE_SVC_CTL_OFFSET, 4)) {
        pcie_svc_apply_gating(dev);
    }
}
