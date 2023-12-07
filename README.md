<h1 align="center">NvStrapsReBar</h1>
<p>This is a copy of the rather popular <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> DXE driver. <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> enables Resizable BAR for older motherboards and chipsets without ReBAR support from the manufacturer. NvStrapsReBar was created to test Resizable BAR support for NVIDIA GPUs from the RTX 2000 (and GTX 1600, Turing architecture) line. Apparently for the GTX 1000 cards (Pascal architecture) the Windows NVIDIA driver just resets the computer during Windows boot if the BAR size has been changed, so GTX 1000 cards still can not enable ReBAR.</p>

Currently the location of the GPU on the PCI bus has to be hard-coded right into the motherboard UEFI, and so does the associated PCI-to-PCI bridge. All hard-coded values are in the header file `ReBarDxe/include/LocalPciGPU.h`, and all the values can be read from the CPU-Z .txt report file, if you want to change them to match your system. Currently these settings are listed below, where all numeric values are examples only (for my computer):

```C
#define TARGET_GPU_PCI_VENDOR_ID        0x10DEu
#define TARGET_GPU_PCI_DEVICE_ID        0x1E07u

#define TARGET_GPU_PCI_BUS              0x41u
#define TARGET_GPU_PCI_DEVICE           0x00u
#define TARGET_GPU_PCI_FUNCTION         0x00u
#define TARGET_GPU_BAR0_ADDRESS         UINT32_C(0x82000000)               // Should fall within memory range mapped by the bridge 

#define TARGET_GPU_BAR1_SIZE_SELECTOR   _16G                               // Desired size for GPU BAR1, should cover the VRAM size

// Secondary bus of the bridge must match the GPU bus
// Check the output form CPU-Z .txt report

#define TARGET_BRIDGE_PCI_VENDOR_ID     0x1022u
#define TARGET_BRIDGE_PCI_DEVICE_ID     0x1453u

#define TARGET_BRIDGE_PCI_BUS           0x40u
#define TARGET_BRIDGE_PCI_DEVICE        0x03u
#define TARGET_BRIDGE_PCI_FUNCTION      0x01u

// Memory range and I/O port range (base + limit) mapped to bridge
// from CPU-Z .txt report of the bridge and GPU
#define TARGET_BRIDGE_MEM_BASE_LIMIT    UINT32_C(0x83008200)            // From offset 0x20 into the PCI config registers of the bridge,
                                                                        // read as little-endian (reverse the byte order)
                                                                        // The the range of values should cover the GPU BAR0 address
#define TARGET_BRIDGE_IO_BASE_LIMIT     0x8181u                         // From offset 0x1C into the PCI config area of the bridge
                                                                        // read as little-endian
```

See https://github.com/xCuri0/ReBarUEFI/discussions/89#discussioncomment-7697768 for the step-by-step details.

Rebuild the project using the instructions below (that were slightly adapted from the original [ReBarUEFI](https://github.com/xCuri0/ReBARUEFI) project).

Credits go to the bellow github users, as I integrated and coded their findings and results:
* [envytools](https://github.com/envytools/envytools) project for the original effort on reverse-engineering the register interface for NVIDIA GPUs, a very long time ago, for use by the [nouveau](https://nouveau.freedesktop.org/) open-source driver in Linux. Amazing how this documentation could still help us today !
* [@mupuf](https://github.com/mupuf) from [envytools](https://github.com/envytools/envytools) project for bringing up the idea and the exact registers from the documentation that could enable resizable BAR
* [@Xelafic](https://github.com/Xelafic) for the first code samples (written in assembly!) and the first test for using the GPU STRAPS bits, documented by envytools, to select the BAR size during PCIe bring-up in UEFI code.
* [@xCuri0](https://github.com/xCuri0/ReBARUEFI") for the ReBarUEFI DXE driver that enables ReBAR on the motherboard, and allows intercepting and hooking into the PCIe enumeration phases in UEFI code on the motherboard.

## Building (Windows only)
* Download and install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/community/) from Microsoft. Be sure to select C/C++ Desktop Development option for installation.
* Download and install [Python 3](https://www.python.org/downloads/)
* Download and install [git](https://git-scm.com/download/win)
* Download and install [CMake](https://cmake.org/download/)
* Install pefile module for python:
   - Use Win+R to open a cmd window and run `pip3 install pefile`
* I also had to install [NASM](https://www.nasm.us/) and add it to the PATH environment variable (in Windows Settings).
* Get `edk2` framework from github. In a cmd window run:
  ```
  ChDir "%UserProfile%
  git clone https://github.com/tianocore/edk2.git
  ChDir edk2
  git submodule update --init
  ```
* Build edk2. Open x86 Native Tools Command Prompt from the start menu (like in the image)
  
  ![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/3e10ca0a-5544-45d3-b2f9-f81a7f7e2510)

  and type the following commands in the resulting console window:
  - `ChDir "%UserProfile%\edk2"`
  - `edksetup.bat Rebuild`
* Configure `edk2`. Edit file `%UserProfile%\edk2\Conf\target.txt` with a text editor like Notepad for example and search, in order, for the lines begining with `TARGET =`, `TARGET_ARCH = ` and `TOOL_CHAIN_TAG =` (without any # characters -- if needed remove the leading # character from such lines, to uncomment them). Modify these lines to read:
  ```
  TARGET                = RELEASE
  TARGET_ARCH           = X64
  TOOL_CHAIN_TAG        = VS2019
  ```
* Get `NvStrapsReBar` from this repository, and place it as a subdirectory right under the `edk2` directory, with the following commands:
  ```
  ChDir "%UserProfile%\edk2"
  git clone https://github.com/terminatorul/NvStrapsReBar.git
  ```
You can now build the UEFI DXE driver `NvStrapsReBar.ffs`, and the Windows executable `NvStrapsReBar.exe`
* To build UEFI DXE driver NvStrapsReBar.ffs, run the following commands in the x86 Native Tools Command Prompt window
  ```
  ChDir "%UserProfile%\edk2"
  If Not Defined EDK_TOOLS_BIN edksetup.bat
  ChDir NvStrapsReBar/ReBarDxe
  python3 buildffs.py
  ```
  The NvStrapsReBar.ffs file will be found under the directory `%UserProfile%\edk2\Build\NvStrapsReBar\RELEASE_VS2019\X64\`.
* To build the Windows executable NvStrapsReBar.exe, run the following commands in the x86 Native Tools Command Prompt window
  ```
  ChDir "%UserProfile%\edk2"
  If Not Defined EDK_TOOLS_BIN edksetup.bat
  ChDir NvStrapsRebar\ReBarState
  MkDir build
  ChDir build
  cmake ..
  cmake --build . --config Release
  ```
  The NvStrapsRebar.exe file will be found under the `%UserProfile%\edk2\NvStrapsRebar\ReBarState\build\Release\` subdirectory

## Updating UEFI
The resulting `NvStrapsReBar.ffs` file needs to be included in the motherboard UEFI image (downloaded from the montherboard manufacturer), and the resulting image should be flashed onto the motherboard as if it were a new UEFI version for that motherboard.
See the original project [ReBarUEFI](https://github.com/xCuri0/ReBarUEFI/) for the instructions to update motherboard UEFI. Replace "ReBarUEFI.ffs" with "NvStrapsReBar.ffs" where appropriate.

After flashing the motherboard with the new UEFI image, you need to enable "Above 4G decoding" and disable CSM in UEFI setup, and then run `NvStrapsReBar.exe` as Administrator, and input a non-zero value when prompted (usually 32 for any possible BAR size)


<p>Bellow is the README page from the original project <a href="https://github.com/xCuri0/ReBarUEFI">xCuri0/ReBarUEFI</a>. Beware links point to the original project wiki as well, so this page may be out of date (out of sync) with the linked wiki pages.</p>


<h1 align="center">ReBarUEFI</h1>
<p align="center">
<a href="https://github.com/xCuri0/ReBarUEFI/actions/workflows/ReBarDxe.yml"><img src="https://img.shields.io/github/actions/workflow/status/xCuri0/ReBarUEFI/ReBarDxe.yml?logo=github&label=ReBarDxe&style=flat-square" alt="GitHub Actions ReBarDxe"></a>
<a href="https://github.com/xCuri0/ReBarUEFI/actions/workflows/ReBarState.yml"><img src="https://img.shields.io/github/actions/workflow/status/xCuri0/ReBarUEFI/ReBarState.yml?logo=github&label=ReBarState&style=flat-square" alt="GitHub Actions ReBarState"></a>
<a href="https://github.com/xCuri0/ReBarUEFI/releases/"><img src="https://img.shields.io/github/downloads/xCuri0/ReBarUEFI/total.svg?logo=github&logoColor=white&style=flat-square&color=E75776" alt="Downloads"></a>
</p>
<p align="center">
A UEFI DXE driver to enable Resizable BAR on systems which don't support it officially. This provides performance benefits and is even <a href="https://www.intel.com/content/www/us/en/support/articles/000092416/graphics.html">required</a> for Intel Arc GPUs to function optimally.
</p>


![screenshot showing cpu-z, gpu-z and amd software](rebar.png)

## Requirements
* (optional) 4G Decoding enabled. See wiki page [Enabling hidden 4G decoding](https://github.com/xCuri0/ReBarUEFI/wiki/Enabling-hidden-4G-decoding) if you can't find an option for it. **Without 4G Decoding you will be limited to 1GB BAR and in some cases 512MB you can try to increase this upto 2GB by reducing TOLUD**
* (optional) BIOS support for Large BARs. Patches exist to fix most issues relating to this

## Usage
Follow the wiki guide [Adding FFS module](https://github.com/xCuri0/ReBarUEFI/wiki/Adding-FFS-module) and continue through the steps. It covers adding the module and the additional modifications needed if required.

Once running the modified firmware make sure that **4G decoding is enabled and CSM is off**.

Next run **ReBarState** which can be found in [Releases](https://github.com/xCuri0/ReBarUEFI/releases) (if you're on Linux build with CMake) and set the Resizable BAR size. In most cases you should be able to use ```32``` (unlimited) without issues but you might need to use a smaller BAR size if ```32``` doesn't work

 **If Resizable BAR works for you reply to [List of working motherboards](https://github.com/xCuri0/ReBarUEFI/issues/11) so I can add it to the list.** Most firmware will accept unsigned/patched modules with Secure Boot on so you won't have any problems running certain games.

If you have any issues after enabling Resizable BAR see [Common Issues (and fixes)](https://github.com/xCuri0/ReBarUEFI/wiki/Common-issues-(and-fixes))

## How it works
The module is added to the UEFI firmware's DXE volume so it gets executed on every boot. The ReBarDxe module replaces the function ```PreprocessController``` of ```PciHostBridgeResourceAllocationProtocol``` with a function that checks for Resizable BAR capability and then sets it to the size from the ```ReBarState``` NVRAM variable after running the original function.

The new ```PreprocessController``` function later gets called during PCI enumeration by the ```PciBus``` module which will detect the new BAR size and allocate it accordingly.

## AliExpress X99 Tutorial by Miyconst
[![Resizable BAR on LGA 2011-3 X99](http://img.youtube.com/vi/vcJDWMpxpjE/0.jpg)](http://www.youtube.com/watch?v=vcJDWMpxpjEE "Resizable BAR on LGA 2011-3 X99")

Instructions for applying UEFIPatch not included as it isn't required for these X99 motherboards. You can follow them below.

## UEFI Patching
Most UEFI firmwares have problems handling 64-bit BARs so several patches were created to fix these issues. You can use [UEFIPatch](https://github.com/LongSoft/UEFITool/releases/tag/0.28.0) to apply these patches located in the UEFIPatch folder. See wiki page [Using UEFIPatch](https://github.com/xCuri0/ReBarUEFI/wiki/Using-UEFIPatch) for more information on using UEFIPatch. **Make sure to check that pad files aren't changed and if they are use the workaround**

### Working patches
* <4GB BAR size limit removal
* <16GB BAR size limit removal
* <64GB BAR size limit removal
* Prevent 64-bit BARs from being downgraded to 32-bit
* Increase MMIO space to 64GB (Haswell/Broadwell). Full 512GB/39-bit isn't possible yet.
* Increase MMIO space from 16GB to full usage of 64GB/36-bit range (Sandy/Ivy Bridge). **Requires DSDT modification on certain motherboards. See wiki page [DSDT Patching](https://github.com/xCuri0/ReBarUEFI/wiki/DSDT-Patching#sandyivy-bridge-dsdt-patch) for more information.**
* Remove NVRAM whitelist to solve ReBarState ```GetLastError: 5```
* Fix USB 3 ports not working in BIOS with 4G Decoding enabled (Ivy Bridge/Haswell/Broadwell)
* X79 Above 4G Decoding fix
  
## Build
Use the provided **buildffs.py** script after cloning inside an [edk2](https://github.com/tianocore/edk2) tree to build the DXE driver. ReBarState can be built on Windows or Linux using CMake. See wiki page [Building](https://github.com/xCuri0/ReBarUEFI/wiki/Building) for more information.

## FAQ
### Will it work on a PCIe Gen2 system ?
Previously it was thought that it won't work on PCIe Gen2 systems but one user had it work with an i5 2500k.

### Can I use Resizable BAR on my system without modifying BIOS ?
You can use Linux with **4G Decoding on**, recent versions will automatically resize and allocate GPU BARs. If your BIOS doesn't have the 4G decoding option (make sure to check [hidden](https://github.com/xCuri0/ReBarUEFI/wiki/Enabling-hidden-4G-decoding)) or DSDT is faulty you can then follow the [Arch wiki guide for DSDT modification](https://wiki.archlinux.org/title/DSDT#Recompiling_it_yourself) using modifications from [DSDT Patching](https://github.com/xCuri0/ReBarUEFI/wiki/DSDT-Patching) and boot with ```pci=realloc``` in your kernel command line. **Currently there is no known method to get it on Windows without BIOS modification**

### I set an unsupported BAR size and my system won't boot
Clear CMOS and Resizable BAR should be disabled. In some cases it may be necessary to remove the CMOS battery for Resizable BAR to disable.

### Will less than optimal BAR sizes still give a performance increase ?
On my system with an i5 3470 and Sapphire Nitro+ RX 580 8GB with [Resizable BAR enabled in driver](https://github.com/xCuri0/ReBarUEFI/wiki/Common-issues-(and-fixes)#how-do-i-enable-resizable-bar-on-unsupported-amd-gpus-) I get an upto 12% FPS increase with 2GB BAR size.

## Credit
* [@dsanke](https://github.com/dsanke), [@cursemex](https://github.com/cursemex), [@val3nt33n](https://github.com/@val3nt33n), [@Mak3rde](https://github.com/Mak3rde) and [@romulus2k4](https://github.com/romulus2k4) for testing/helping develop patches

* The Linux kernel especially the ```amdgpu``` driver

* [EDK2](https://github.com/tianocore/edk2) for the base that all OEM UEFI follows

* [Ghidra](https://ghidra-sre.org/) which was used to patch UEFI modules to workaround artificial limitations

* [@vit9696](https://github.com/vit9696) for the NVRAM whitelist patches

* [@ZOXZX](https://github.com/ZOXZX) for helping with the X79 Above 4G patches

* [@NikolajSchlej](https://github.com/NikolajSchlej) for developing UEFITool/UEFIPatch

* [QEMU](https://www.qemu.org/)/OVMF made testing hooking way easier although it didn't have any resizable BAR devices so the only way I could test it was on my actual PC.
