
#include <stdint.h>

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/PciExpress21.h>

#include "pciRegs.h"
#include "S3ResumeScript.h"
#include "LocalAppConfig.h"
#include "StatusVar.h"
#include "SetupNvStraps.h"
#include "ReBar.h"
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

EFI_STATUS pciReadDeviceSubsystem(UINTN pciAddress, uint_least16_t *subsysVenID, uint_least16_t *subsysDevID)
{
    UINT32 subsys = MAX_UINT32;
    EFI_STATUS status = pciReadConfigDword(pciAddress, PCI_SUBSYSTEM_VENDOR_ID_OFFSET, &subsys);

    if (EFI_ERROR(status))
        *subsysVenID = WORD_BITMASK, *subsysDevID = WORD_BITMASK;
    else
        *subsysVenID = subsys & WORD_BITMASK, *subsysDevID = subsys >> WORD_BITSIZE & WORD_BITMASK;

    return status;
}

uint_least32_t pciDeviceClass(UINTN pciAddress)
{
    UINT32 configReg;

    if (EFI_ERROR(pciReadConfigDword(pciAddress, PCI_REVISION_ID_OFFSET, &configReg)))
    return UINT32_C(0xFFFF'FFFF);

    return configReg & UINT32_C(0xFFFF'FF00);
}

uint_least32_t pciDeviceBAR0(UINTN pciAddress, EFI_STATUS *status)
{
    UINT32 baseAddress;

    *status = pciReadConfigDword(pciAddress, PCI_BASE_ADDRESS_0, &baseAddress);

    if (EFI_ERROR(*status))
    return UINT32_C(0xFFFF'FFFF);

    return baseAddress;
}

EFI_STATUS pciBridgeSecondaryBus(UINTN pciAddress, uint_least8_t *secondaryBus)
{
    UINT32 configReg;

    EFI_STATUS status = pciReadConfigDword(pciAddress, PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET, &configReg);

    if (EFI_ERROR(status))
    *secondaryBus = BYTE_BITMASK;
    else
    *secondaryBus = configReg >> BYTE_BITSIZE & BYTE_BITMASK;

    return status;
}

bool pciIsPciBridge(uint_least8_t headerType)
{
    return (headerType & ~HEADER_TYPE_MULTI_FUNCTION) == (uint_least8_t) HEADER_TYPE_PCI_TO_PCI_BRIDGE;
}

bool pciIsVgaController(uint_least32_t pciClassReg)
{
    return pciClassReg ==
         ((uint_least32_t)PCI_CLASS_DISPLAY     << 3u * BYTE_BITSIZE
        | (uint_least32_t)PCI_CLASS_DISPLAY_VGA << 2u * BYTE_BITSIZE
        | (uint_least32_t)PCI_IF_VGA_VGA	    << 1u * BYTE_BITSIZE);
}

UINTN pciLocateDevice(EFI_HANDLE RootBridgeHandle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addressInfo, uint_least16_t *venID, uint_least16_t *devID, uint_least8_t *headerType)
{
    gBS->HandleProtocol(RootBridgeHandle, &gEfiPciRootBridgeIoProtocolGuid, (void **)&pciRootBridgeIo);

    UINTN pciAddress = EFI_PCI_ADDRESS(addressInfo.Bus, addressInfo.Device, addressInfo.Function, 0x00u);
    UINT32 pciID;

    pciReadConfigDword(pciAddress, PCI_VENDOR_ID_OFFSET, &pciID);

    if (pciID != ((uint_least32_t) WORD_BITMASK << WORD_BITSIZE | WORD_BITMASK))
    {
    UINT32 configReg;

    if (EFI_ERROR(pciReadConfigDword(pciAddress, PCI_CACHELINE_SIZE_OFFSET, &configReg)))
        *headerType = BYTE_BITMASK;
    else
        *headerType = (configReg >> WORD_BITSIZE) & BYTE_BITMASK;
    }
    else
    *headerType = BYTE_BITMASK;

    *venID = pciID & WORD_BITMASK;
    *devID = pciID >> WORD_BITSIZE & WORD_BITMASK;

    return pciAddress;
}

// adapted from Linux pci_find_ext_capability
uint_least16_t pciFindExtCapability(UINTN pciAddress, uint_least32_t cap)
{
    uint_least16_t capabilityOffset = EFI_PCIE_CAPABILITY_BASE_OFFSET;
    UINT32 capabilityHeader;
    EFI_STATUS status;

    if (EFI_ERROR((status = pciReadConfigDword(pciAddress, capabilityOffset, &capabilityHeader))))
        return SetEFIError(EFIError_PCI_StartFindCap, status), 0u;

    /*
     * If we have no capabilities, this is indicated by cap ID,
     * cap version and next pointer all being 0. Or it could also be all FF
     */
    if (capabilityHeader == 0u || PCI_POSSIBLE_ERROR(capabilityHeader))
        return 0u;

    /* minimum 8 bytes per capability */
    int_fast16_t  ttl = (PCI_CFG_SPACE_EXP_SIZE - EFI_PCIE_CAPABILITY_BASE_OFFSET) / 8u;

    while (ttl-- > 0)
    {
        if (PCI_EXT_CAP_ID(capabilityHeader) == cap && capabilityOffset)
            return capabilityOffset;

        capabilityOffset = PCI_EXT_CAP_NEXT(capabilityHeader);

        if (capabilityOffset < EFI_PCIE_CAPABILITY_BASE_OFFSET)
            break;

        if (EFI_ERROR((status = pciReadConfigDword(pciAddress, capabilityOffset, &capabilityHeader))))
        {
            SetEFIError(EFIError_PCI_FindCap, status);
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

    return 0u;
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

void pciSaveAndRemapBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[3u], EFI_PHYSICAL_ADDRESS baseAddress0, EFI_PHYSICAL_ADDRESS topAddress0, EFI_PHYSICAL_ADDRESS ioBaseLimit)
{
    bool efiError = false, s3SaveStateError = false;
    EFI_STATUS status;

    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(bridgePciAddress, PCI_COMMAND_OFFSET, bridgeSaveArea + 0u)));
    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(bridgePciAddress, PCI_IO_BASE,        bridgeSaveArea + 1u)));
    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(bridgePciAddress, PCI_MEMORY_BASE,    bridgeSaveArea + 2u)));

    UINT32 configReg;
    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(bridgePciAddress, PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET, &configReg)));

    if (!efiError)
    {
    topAddress0++;

    if (topAddress0 & UINT32_C(0x000F'FFFF))
    {
        topAddress0 += UINT32_C(0x0010'0000);	// round up to next 1 MiByte alignment
        topAddress0 &= UINT32_C(0xFFF0'0000);
    }

    if (topAddress0 <= baseAddress0)
    {
        SetStatusVar(StatusVar_BadBridgeConfig);
        return;
    }

        UINT32 bridgeIoRange = ioBaseLimit & 0xFF00u | ioBaseLimit >> BYTE_BITSIZE & 0x00FFu;
        UINT32
                bridgeCommand = bridgeSaveArea[0u] | EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER,
                bridgeIoBaseLimit = bridgeSaveArea[1u] & UINT32_C(0xFFFF'0000) | bridgeIoRange & UINT32_C(0x0000'FFFF),
                bridgeMemoryBaseLimit = (baseAddress0 >> 16u & UINT32_C(0x0000'FFF0) | topAddress0 & UINT32_C(0xFFF0'0000));

        efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE,    &bridgeMemoryBaseLimit)));
        efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,        &bridgeIoBaseLimit)));
        efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_COMMAND_OFFSET, &bridgeCommand)));

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigReadWrite_DWORD
        (
            bridgePciAddress,
            PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET,
            configReg & UINT32_C(0x00FF'FFFF),	    // primary bus, secondary bus, subsidiary bus
            UINT32_C(0xFF00'0000)
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigWrite_DWORD
        (
            bridgePciAddress,
            PCI_MEMORY_BASE,
            bridgeMemoryBaseLimit
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigReadWrite_DWORD
        (
            bridgePciAddress,
            PCI_IO_BASE,
            bridgeIoBaseLimit & UINT32_C(0x0000'FFFF),
            UINT32_C(0xFFFF'0000)
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigReadWrite_DWORD
        (
            bridgePciAddress,
            PCI_COMMAND_OFFSET,
            EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER,
            (UINT32) ~(UINT32)(EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER)
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }
    }

    if (efiError)
        SetEFIError(s3SaveStateError ? EFIError_WriteS3SaveStateProtocol : EFIError_PCI_BridgeConfig, status);
}

void pciRestoreBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[3u])
{
    bool efiError = false;
    EFI_STATUS status;

    efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_COMMAND_OFFSET, bridgeSaveArea + 0u)));
    efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,        bridgeSaveArea + 1u)));
    efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE,    bridgeSaveArea + 2u)));

    if (efiError)
        SetEFIError(EFIError_PCI_BridgeRestore, status);
}

void pciSaveAndRemapDeviceBAR0(UINTN pciAddress, UINT32 gpuSaveArea[2u], EFI_PHYSICAL_ADDRESS baseAddress0)
{
    bool efiError = false, s3SaveStateError = false;
    EFI_STATUS status;

    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(pciAddress, PCI_COMMAND_OFFSET, gpuSaveArea + 0u)));
    efiError = efiError || EFI_ERROR((status = pciReadConfigDword(pciAddress, PCI_BASE_ADDRESS_0, gpuSaveArea + 1u)));

    if (!efiError)
    {
    if (baseAddress0 & UINT32_C(0x0000'000F))
    {
        SetDeviceStatusVar(pciAddress, StatusVar_BadGpuConfig);
        return;
    }

        UINT32
           gpuBaseAddress = baseAddress0 & UINT32_C(0xFFFFFFF0),
           gpuCommand = gpuSaveArea[0u] | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;

        efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(pciAddress, PCI_BASE_ADDRESS_0, &gpuBaseAddress)));
        efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(pciAddress, PCI_COMMAND_OFFSET, &gpuCommand)));

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigWrite_DWORD
        (
            pciAddress,
            PCI_BASE_ADDRESS_0,
            gpuBaseAddress
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }

    if (!efiError)
    {
        status = S3ResumeScript_PciConfigReadWrite_DWORD
        (
            pciAddress,
            PCI_COMMAND_OFFSET,
            EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER,
            (uint_least32_t) ~(uint_least32_t)(EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER)
        );

        efiError = efiError || EFI_ERROR(status);
        s3SaveStateError = s3SaveStateError || EFI_ERROR(status);
    }
    }

    if (efiError)
        SetEFIError(s3SaveStateError ? EFIError_WriteS3SaveStateProtocol : EFIError_PCI_DeviceBARConfig, status);
}

void pciRestoreDeviceConfig(UINTN pciAddress, UINT32 saveArea[2u])
{
    bool efiError = false;
    EFI_STATUS status;

    efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(pciAddress, PCI_COMMAND_OFFSET, saveArea + 0u)));
    efiError = efiError || EFI_ERROR((status = pciWriteConfigDword(pciAddress, PCI_BASE_ADDRESS_0, saveArea + 1u)));

    if (efiError)
        SetEFIError(EFIError_PCI_DeviceBARRestore, status);
}

// vim: ft=cpp
