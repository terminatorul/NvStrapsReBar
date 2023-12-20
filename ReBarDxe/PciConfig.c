
#include <stdint.h>

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/PciExpress21.h>

#include "pciRegs.h"
#include "StatusVar.h"
#include "LocalAppConfig.h"
#include "SetupNvStraps.h"
#include "PciConfig.h"

inline bool PCI_POSSIBLE_ERROR(UINT32 val)
{
    return val == MAX_UINT32;
};

static EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *pciRootBridgeIo;

UINT64 pciAddrOffset(UINTN pciAddress, INTN offset)
{
    UINTN reg = (pciAddress & 0xffffffff00000000) >> 32;
    UINTN bus = (pciAddress & 0xff000000) >> 24;
    UINTN dev = (pciAddress & 0xff0000) >> 16;
    UINTN func = (pciAddress & 0xff00) >> 8;

    return EFI_PCI_ADDRESS(bus, dev, func, ((INT64)reg + offset));
}

// created these functions to make it easy to read as we are adapting alot of code from Linux
static inline EFI_STATUS pciReadConfigDword(UINTN pciAddress, INTN pos, UINT32 *buf)
{
    return pciRootBridgeIo->Pci.Read(pciRootBridgeIo, EfiPciWidthUint32, pciAddrOffset(pciAddress, pos), 1u, buf);
}

// Using the PollMem function silently breaks UEFI boot (the board needs flash recovery...)
static inline EFI_STATUS pciPollConfigDword(UINTN pciAddress, INTN pos, UINT64 mask, UINT64 value, UINT64 delay, UINT64 *result)
{
    return pciRootBridgeIo->PollMem(pciRootBridgeIo, EfiPciWidthUint32, pciAddrOffset(pciAddress, pos), mask, value, delay, result);
}

static inline EFI_STATUS pciWriteConfigDword(UINTN pciAddress, INTN pos, UINT32 *buf)
{
    return pciRootBridgeIo->Pci.Write(pciRootBridgeIo, EfiPciWidthUint32, pciAddrOffset(pciAddress, pos), 1u, buf);
}

static inline EFI_STATUS pciReadConfigWord(UINTN pciAddress, INTN pos, UINT16 *buf)
{
    return pciRootBridgeIo->Pci.Read(pciRootBridgeIo, EfiPciWidthUint16, pciAddrOffset(pciAddress, pos), 1u, buf);
}

static inline EFI_STATUS pciWriteConfigWord(UINTN pciAddress, INTN pos, UINT16 *buf)
{
    return pciRootBridgeIo->Pci.Write(pciRootBridgeIo, EfiPciWidthUint16, pciAddrOffset(pciAddress, pos), 1u, buf);
}

static inline EFI_STATUS pciReadConfigByte(UINTN pciAddress, INTN pos, UINT8 *buf)
{
    return pciRootBridgeIo->Pci.Read(pciRootBridgeIo, EfiPciWidthUint8, pciAddrOffset(pciAddress, pos), 1u, buf);
}

static inline EFI_STATUS pciWriteConfigByte(UINTN pciAddress, INTN pos, UINT8 *buf)
{
    return pciRootBridgeIo->Pci.Write(pciRootBridgeIo, EfiPciWidthUint8, pciAddrOffset(pciAddress, pos), 1u, buf);
}

UINTN pciLocatedDevice(EFI_HANDLE RootBridgeHandle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addressInfo, UINT16 *venID, UINT16 *devID)
{
    gBS->HandleProtocol(RootBridgeHandle, &gEfiPciRootBridgeIoProtocolGuid, (void **)&pciRootBridgeIo);

    UINTN pciAddress = EFI_PCI_ADDRESS(addressInfo.Bus, addressInfo.Device, addressInfo.Function, 0u);

    pciReadConfigWord(pciAddress, PCI_VENDOR_ID_OFFSET, venID);
    pciReadConfigWord(pciAddress, PCI_DEVICE_ID_OFFSET, devID);

    return pciAddress;
}

// adapted from Linux pci_find_ext_capability
uint_least16_t pciFindExtCapability(UINTN pciAddress, uint_least32_t cap)
{
    uint_least16_t capabilityOffset = EFI_PCIE_CAPABILITY_BASE_OFFSET;
    UINT32 capabilityHeader;

    if (EFI_ERROR(pciReadConfigDword(pciAddress, capabilityOffset, &capabilityHeader)))
        return SetStatusVar(StatusVar_EFIError), 0u;

    /*
     * If we have no capabilities, this is indicated by cap ID,
     * cap version and next pointer all being 0. Or it could also be all FF
     */
    if (capabilityHeader == 0u || PCI_POSSIBLE_ERROR(capabilityHeader))
        return SetStatusVar(StatusVar_EFIError), 0u;

    /* minimum 8 bytes per capability */
    int_fast16_t  ttl = (PCI_CFG_SPACE_EXP_SIZE - EFI_PCIE_CAPABILITY_BASE_OFFSET) / 8u;

    while (ttl-- > 0)
    {
        if (PCI_EXT_CAP_ID(capabilityHeader) == cap && capabilityOffset)
            return capabilityOffset;

        capabilityOffset = PCI_EXT_CAP_NEXT(capabilityHeader);

        if (capabilityOffset < EFI_PCIE_CAPABILITY_BASE_OFFSET)
            break;

        if (EFI_ERROR(pciReadConfigDword(pciAddress, capabilityOffset, &capabilityHeader)))
        {
            SetStatusVar(StatusVar_EFIError);
            break;
        }
    }

    return 0u;
}

static uint_least16_t pciBARConfigOffset(UINTN pciAddress, uint_least16_t capOffset, uint_least8_t barIndex)
{
    UINT32 configValue;
    pciReadConfigDword(pciAddress, capOffset + PCI_REBAR_CTRL, &configValue);

    unsigned nBars = (configValue & PCI_REBAR_CTRL_NBAR_MASK) >> PCI_REBAR_CTRL_NBAR_SHIFT;

    for (unsigned i = 0u; i < nBars; i++, capOffset += 8u)
    {
        pciReadConfigDword(pciAddress, capOffset + PCI_REBAR_CTRL, &configValue);

        if ((configValue & PCI_REBAR_CTRL_BAR_IDX) == barIndex)
            return capOffset;
    }

    return SetStatusVar(StatusVar_EFIError), 0u;
}

uint_least32_t pciRebarGetPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, UINT16 vid, UINT16 did, uint_least8_t barIndex)
{
    uint_least16_t barConfigOffset = pciBARConfigOffset(pciAddress, capabilityOffset, barIndex);

    if (barConfigOffset)
    {
        UINT32 barSizeMask;
        pciReadConfigDword(pciAddress, barConfigOffset + PCI_REBAR_CAP, &barSizeMask);
        barSizeMask &= PCI_REBAR_CAP_SIZES;

        return barSizeMask >> 4u;
    }

    return 0u;
}

/*
 * This broke UEFI boot (the board won't POST)
uint_least32_t pciRebarPollPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least32_t barSizeMask)
{
    uint_least16_t barConfigOffset = pciBARConfigOffset(pciAddress, capabilityOffset, barIndex);

    if (barConfigOffset)
    {
        UINT64 resultSizeMask;
        pciPollConfigDword(pciAddress, barConfigOffset + PCI_REBAR_CAP, barSizeMask, barSizeMask, UINT64_C(1'000'000), &resultSizeMask);
        barSizeMask &= PCI_REBAR_CAP_SIZES;

        return (uint_least32_t)(barSizeMask) >> 4u;
    }

    return SetStatusVar(StatusVar_EFIError), 0u;
}
 */

bool pciRebarSetSize(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least8_t barSizeBitIndex)
{
    uint_least16_t barConfigOffset = pciBARConfigOffset(pciAddress, capabilityOffset, barIndex);

    if (barConfigOffset)
    {
        UINT32 barSizeControl;
        pciReadConfigDword(pciAddress, barConfigOffset + PCI_REBAR_CTRL, &barSizeControl);

        barSizeControl &= ~ (uint_least32_t)PCI_REBAR_CTRL_BAR_SIZE;
        barSizeControl |= (uint_least32_t)barSizeBitIndex << PCI_REBAR_CTRL_BAR_SHIFT;

        pciWriteConfigDword(pciAddress, barConfigOffset + PCI_REBAR_CTRL, &barSizeControl);

        return true;
    }

    return false;
}

void pciSaveAndRemapBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u], EFI_PHYSICAL_ADDRESS baseAddress0, EFI_PHYSICAL_ADDRESS bridgeBaseIo, UINT8 busNo)
{
    bool efiError = false;

    efiError = efiError || EFI_ERROR(pciReadConfigDword(bridgePciAddress, PCI_COMMAND,     bridgeSaveArea + 0u));
    efiError = efiError || EFI_ERROR(pciReadConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, bridgeSaveArea + 1u));
    efiError = efiError || EFI_ERROR(pciReadConfigDword(bridgePciAddress, PCI_IO_BASE,     bridgeSaveArea + 2u));
    efiError = efiError || EFI_ERROR(pciReadConfigDword(bridgePciAddress, PCI_MEMORY_BASE, bridgeSaveArea + 3u));

    if (!efiError)
    {
        UINT32 bridgeIoRange = (bridgeBaseIo & 0xFF00u | bridgeBaseIo >> 8u & 0x00FFu) & 0xFFFFu;

        UINT32
                bridgeMemoryBaseLimit = (baseAddress0 >> 16u & 0x0000FFFFu | baseAddress0 + SIZE_32MB & 0xFFFF0000u) & 0xFFFFFFFFu,
                bridgeIoBaseLimit = bridgeSaveArea[2u] & UINT32_C(0xFFFF0000) | bridgeIoRange & UINT32_C(0x0000FFFF),
                bridgePciBusNumber = bridgeSaveArea[1u] & UINT32_C(0xFF0000FF) | (busNo & UINT8_C(0xFF)) << 8u | (busNo & UINT8_C(0xFF)) << 16u,
                bridgeCommand = bridgeSaveArea[0u] | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;

        efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE, &bridgeMemoryBaseLimit));
        efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,     &bridgeIoBaseLimit));
        efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, &bridgePciBusNumber));
        efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_COMMAND,     &bridgeCommand));
    }

    if (efiError)
        SetStatusVar(StatusVar_EFIError);
}

void pciRestoreBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u])
{
    bool efiError = false;

    efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_COMMAND,     bridgeSaveArea + 0u));
    efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, bridgeSaveArea + 1u));
    efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,     bridgeSaveArea + 2u));
    efiError = efiError || EFI_ERROR(pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE, bridgeSaveArea + 3u));

    if (efiError)
        SetStatusVar(StatusVar_EFIError);
}

void pciSaveAndRemapDeviceBAR0(UINTN pciAddress, UINT32 gpuSaveArea[2u], EFI_PHYSICAL_ADDRESS baseAddress0)
{
    bool efiError = false;

    efiError = efiError || EFI_ERROR(pciReadConfigDword(pciAddress, PCI_COMMAND,        gpuSaveArea + 0u));
    efiError = efiError || EFI_ERROR(pciReadConfigDword(pciAddress, PCI_BASE_ADDRESS_0, gpuSaveArea + 1u));

    if (!efiError)
    {
        UINT32
           gpuBaseAddress = baseAddress0 & 0xFFFFFFFFu,
           gpuCommand = gpuSaveArea[0u] | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;

        efiError = efiError || EFI_ERROR(pciWriteConfigDword(pciAddress, PCI_BASE_ADDRESS_0, &gpuBaseAddress));
        efiError = efiError || EFI_ERROR(pciWriteConfigDword(pciAddress, PCI_COMMAND,        &gpuCommand));
    }

    if (efiError)
        SetStatusVar(StatusVar_EFIError);
}

void pciRestoreDeviceConfig(UINTN pciAddress, UINT32 saveArea[2u])
{
    bool efiError = false;

    efiError = efiError || EFI_ERROR(pciWriteConfigDword(pciAddress, PCI_COMMAND,        saveArea + 0u));
    efiError = efiError || EFI_ERROR(pciWriteConfigDword(pciAddress, PCI_BASE_ADDRESS_0, saveArea + 1u));

    if (efiError)
        SetStatusVar(StatusVar_EFIError);
}
