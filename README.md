<h1 align="center">NvStrapsReBar</h1>
<p>UEFI driver to enable and test Resizable BAR on Turing graphics cards (GTX 1600, RTX 2000). Pre-Pascal cards might also work.</p>
<p>This is a copy of the rather popular <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> DXE driver. <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> enables Resizable BAR for older motherboards and chipsets without ReBAR support from the manufacturer. NvStrapsReBar was created to test Resizable BAR support for GPUs from the RTX 2000 (and GTX 1600, Turing architecture) line. Apparently for the GTX 1000 cards (Pascal architecture) the Windows driver resets the computer during boot if the BAR size has been changed, so GTX 1000 cards still can not enable ReBAR. The Linux driver does not crash, but does not pick up the new BAR size either.</p>

### Do I need to flash a new UEFI image on the motherboard, to enable ReBAR on the GPU ?
Yes, this is how it works for Turing GPUs (GTX 1600 / RTX 2000).

(some ideas to get it working without UEFI modding have circulated, but may not be technically possible and nothing is implemented.)

It's ususally the video BIOS (vBIOS) that should enable ReBAR, but the vBIOS is digitally signed (NVIDIA vBIOS is also encrypted) and can not be modified by modders and end-users (is locked-down). The motherboard UEFI image can also be signed or have integrity checks, but in general it is thankfully not as locked down, and users and UEFI modders often still have a way to modify it.

For older boards without ReBAR, adding ReBAR functionality depends on the Above 4G Decoding option in your UEFI setup, which must be turned on in advance, and CSM must be disabled.

#### Warning:
Some users report BSOD or crash when resuming from sleep, which can be even worse for laptop users. Developing a boot script for the DXE driver might address the issue, but the feature is not implemented and there is currently no fix for this issue.

### Usage
Download latest release from the [Releases](https://github.com/terminatorul/NvStrapsReBar/releases) page, or build the project using the  [build](https://github.com/terminatorul/NvStrapsReBar/wiki/Building-(Windows-only)) instructions. This should produce two files:
* `NvStrapsReBar.ffs` UEFI DXE driver
* `NvStrapsReBar.exe` Windows executable

After download or build you need to go through a number of steps:
* update the motherbord UEFI image to add the new `NvStrapsReBar.ffs` driver (see below)
* enable ReBAR in UEFI Setup if the motherboard supports it. Otherwise enable "Above 4G Decoding" (if you have the option) and disable CSM
* run `NvStrapsReBar.exe` as Administrator to enable the new BAR size, by following the text-mode menus. If you have a recent motherboard, you only need to input `E` to Enable ReBAR for Turing GPUs, then input `S` to save the new driver configuration to EFI variable. For older motherboards without ReBAR, you also need to input `P` to set BAR size on the PCI side (motherboard side).
* reboot after saving the menu options.
* for older motherboards without ReBAR, if you want to load default UEFI settings again, or disable Above 4G Decoding / enable CSM, you need to  disable ReBAR first in `NvStrapsReBar.exe`. Or you can manually set back the current year in UEFI Setup.

![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/21da2dc9-82be-4ac6-8e60-2f61bd619f0a)


Credits go to the bellow github users, as I integrated and coded their findings and results:
* [envytools](https://github.com/envytools/envytools) project for the original effort on reverse-engineering the register interface for the GPUs, a very long time ago, for use by the [nouveau](https://nouveau.freedesktop.org/) open-source driver in Linux. Amazing how this documentation could still help us today !
* [@mupuf](https://github.com/mupuf) from [envytools](https://github.com/envytools/envytools) project for bringing up the idea and the exact (low level) registers from the documentation, that enable resizable BAR
* [@Xelafic](https://github.com/Xelafic) for the first code samples (written in assembly!) and the first test for using the GPU STRAPS bits, documented by envytools, to select the BAR size during PCIe bring-up in UEFI code.
* [@xCuri0](https://github.com/xCuri0/ReBARUEFI") for great support and for the ReBarUEFI DXE driver that enables ReBAR on the motherboard, and allows intercepting and hooking into the PCIe enumeration phases in UEFI code on the motherboard.

## Working GPUs
Check issue https://github.com/terminatorul/NvStrapsReBar/issues/1 for a list of known working GPUs (and motherboards).

If you get Resizable BAR working on your Turing (or earlier) GPU, please post your system information on issue https://github.com/terminatorul/NvStrapsReBar/issues/1 here on github,
in the below format

* CPU:
* Motherboard model:
* Motherboard chipset:
* Graphics card model:
* GPU chipset:
* GPU PCI VendorID:DeviceID (check GPU-Z):
* GPU PCI subsystem IDs (check GPU-Z):
* VRAM size:
* New BAR size (GPU-Z):
* New BAR size (nvidia-smi):
* driver version:

Use command `nvidia-smi -q -d memory` to check the new BAR size reported by the Windows/Linux driver.

It maybe easier and more informative to post GPU-Z screenshots with the main GPU page + ReBAR page, and CPU-X with the CPU page and motherboard page screenshots, plus the output from nvidia-smi command. If you needed to apply more changes to make ReBAR work, please post about them as well.

## Updating UEFI image

You can download the latest release from the [Releases](https://github.com/terminatorul/NvStrapsReBar/releases) page, or build the UEFI DXE driver and the Windows executable using the instructions on the [building](https://github.com/terminatorul/NvStrapsReBar/wiki/Building-(Windows-only)) page.

The resulting `NvStrapsReBar.ffs` file needs to be included in the motherboard UEFI image (downloaded from the montherboard manufacturer), and the resulting image should be flashed onto the motherboard as if it were a new UEFI version for that board.
See the original project [ReBarUEFI](https://github.com/xCuri0/ReBarUEFI/) for the instructions to update motherboard UEFI. Replace "ReBarUEFI.ffs" with "NvStrapsReBar.ffs" where appropriate.

<p>So you will still have to check the README page from the original project: <ul><li><a href="https://github.com/xCuri0/ReBarUEFI">https://github.com/xCuri0/ReBarUEFI</a></li></ul> for all the details and instructions on working with the UEFI image, and patching it if necessary (for older motherboards and chipsets). </p>

## Enable ReBAR and choose BAR size
After flashing the motherboard with the new UEFI image, you need to enable ReBAR in UEFI Setup. For older motherboards without ReBAR, enable "Above 4G Decoding" and disable CSM. Then you need to run `NvStrapsReBar.exe` as Administrator.

For older motherboard without ReBAR support, enablging ReBAR depends on Above 4G Decoding. So if you accidentaly turn it off later and can not POST, you need to clear CMOS. Remember to disconnect from wall power before you clear CMOS (bad things happened to my motherboard otherwise). Users report the Clear CMOS button present on some motherboards may still keep the current date and time for some boards. The current date has to be reset to recover the board. So if needed, you should short the Clear CMOS pin headers (with a screwdriver that is metallic, and the metal is not painted / coated), or by removing the battery for 1 to 5 minutes. Another way is to move the GPU to a different PCI slot, or replace it with a different model, if you have an extra. If you can enter UEFI Setup, you can manually set back the year to a value before 2024, reboot, and restore the year after.

`NvStrapsReBar.exe` prompts you with a small text-based menu. You can configure 2 value for the BAR size with this tool:
* GPU-side BAR size
* PCI BAR size (for older motherboards without ReBAR)

Newer boards can configure PCI BAR size, so you only need to set the GPU-side value for the BAR size. If not, you should try and experiment with both of them, as needed.

![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/a960adff-665f-4fbb-92ba-a8a4114996ca)



Most people should choose the first menu option and press `E` to Enable auto-settings BAR size for Turing GPUs. Depending on your board, you may need to also input `P` at the menu prompt, to choose Target PCI BAR size, and select value 64 (for the option to configure PCI BAR for selected GPUs only). Before quitting the menu, input `S` to save the changes you made to the EFI variable store, for the UEFI DXE driver to read them.

If you choose a GPU BAR size of 8 GiB for example, and a Target PCI BAR size of 4 GiB, you will get a 4 GiB BAR.

For older boards without ReBAR support from the manufacturer, you can select other values for Target PCI BAR size, to also configure other GPUs for example. Or to limit the BAR size to smaller values even if the GPU supports higher values. Depending on the motherboard UEFI, for some boards you may need to use lower values, to limit BAR size to 4 GB or 2GB for example. Even a 2 GB BAR size still gives you the benefits of Resizable BAR in most titles, and NVIDIA tends to use 1.5 GB as the default size in the Profile Inspector. There are exceptions to this 'though (for some titles that can still see improvements with the higher BAR sizes).

## Using large BAR sizes
Remember you need to use the [Profile Inspector](https://github.com/Orbmu2k/nvidiaProfileInspector) to enable ReBAR per-application, and if neeeded also globally. There appears to be a fake site for the Profile Inspector, so always downloaded it from github, or use the link above.

