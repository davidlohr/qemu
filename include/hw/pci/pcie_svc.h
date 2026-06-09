/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * PCIe Streamlined Virtual Channel (SVC) Extended Capability
 *
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 */

#ifndef HW_PCIE_SVC_H
#define HW_PCIE_SVC_H

#include "hw/pci/pci.h"
#include "qemu/bitops.h"

/*
 * The PCIe config space starts from 0x00 till 0xFF. In that
 * extended capability starts from 0x100 and each extended capability
 * is of dword size (32 bit). So we have to give a proper base offset
 * value which should be >= 0x100 and <= 0xFF and in between other
 * capability will also be registerd. We might not know where exactly
 * SVC extended capability will sit, so to avoid the overlapping we have
 * to give a very high offset.
 */
#define PCI_EXT_CAP_BASE_OFFSET                0x200
#define PCI_EXT_CAP_ID_SVC                     0x35
#define PCI_EXT_CAP_SVC_SIZE                   0x74

/* PCIe 6.4 section 7.9.29 */
#define PCIE_SVC_CAP_HEAD_OFFSET               0x00
#define PCIE_SVC_CAP_OFFSET                    0x04
#define PCIE_SVC_CTL_OFFSET                    0x0c
#define PCIE_SVC_STA_OFFSET                    0x10
#define PCIE_SVC_CTL_ENABLE                    BIT(0)
#define PCIE_SVC_CAP1_EVCC                     (0x7 << 0)

/* 7.9.29.1 SVC extended capability header  */
#define PCIE_SVC_CAP_VER                       (1 << 16)
#define NEXT_CAP_OFF                           (0 << 20)

/* 7.9.29.6 SVC Resource capability Register */
#define SVC_RES_CAP_BASE                       0x14
#define SVC_RES_CAP(n)                         (SVC_RES_CAP_BASE + (n) * 0x0c)

/*
 * As per Specification SVC VC3 is dedicated to UIO and non-UIO traffic cannot
 * use that, so for SVC VC3 the value would be 0010. SVC VC4 is an optional VC
 * for UIO and VC4 can be used by non-UIO traffic as well. So the protocol for
 * VC4 would be 0011. For SVC VC0, the protocol is 0000. Rest are all reserved
 * asper table 2-46 in PCIe 6.4 specification.
 */
#define SVC_VC0_PROTOCOL                       (0x0 << 8)
#define SVC_VC3_PROTOCOL                       (0x2 << 8)
#define SVC_VC4_PROTOCOL                       (0x3 << 8)
#define SVC_VC_ID(n)                           ((n & 0x7) << 12)

/* 7.9.27.7 SVC Resource Control Register */
#define SVC_RES_CTRL_BASE                      0x18
#define SVC_RES_CTRL(n)                        (SVC_RES_CTRL_BASE + (n) * 0x0c)
#define SVC_VC_ENABLE                          BIT(31)
#define SHARED_FLOW_CONTROL_USAGE_LIMIT_ENABLE BIT(30)
#define SVC_VC_PROTOCOL_SELECTED               (0xf << 8)
#define SVC_UIO_PROTOCOL_SELECTED              (0x2 << 8)
/*
 * As per Table 7-350 in 7.9.29.7 in PCIe 6.4 Specification, the protocol
 * selected should be 0010 for UIO enabled VCs
 */
#define SHARED_FLOW_CONTROL_USAGE_LIMIT        (0x7 << 27)
#define SVC_TC_VC_MAP_ENABLE                   (0xff << 0)
#define SVC_TC_VC_MAP(n)                       ((n & 0xff) << 0)

/* 7.9.27.8 SVC Resource Status Register */
#define SVC_RES_STATUS_BASE                    0x1c
#define SVC_RES_STATUS(n)                      (SVC_RES_STATUS_BASE + \
                                                (n) * 0x0c)
#define SVC_VC_NEGO                            BIT(1)

typedef struct PCIESvcCap {
    uint32_t ctrl;
    uint32_t status;
    bool uio_mand_svc;
    bool uio_opt_svc;
} PCIESvcCap;

int pcie_config_uio_svc(PCIDevice *d, Error **errp);
int  pcie_svc_cap_init(PCIDevice *dev, uint16_t offset, Error **errp);
void pcie_svc_cap_reset(PCIDevice *dev);
void pcie_svc_set_vc4(PCIDevice *dev, bool enable);
void pcie_svc_cap_write_config(PCIDevice *dev,
                               uint32_t addr, uint32_t val, int len);

#endif /* HW_PCIE_SVC_H */
