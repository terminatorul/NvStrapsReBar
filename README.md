<h1 align="center">NvStrapsReBar</h1>
<p>UEFI driver to test and enable Resizable BAR on Turing graphics cards (GTX 1600, RTX 2000). Pre-Pascal cards might also work.</p>
<p>This is a copy of the rather popular <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> DXE driver. <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> enables Resizable BAR for older motherboards and chipsets without ReBAR support from the manufacturer. NvStrapsReBar was created to test Resizable BAR support for GPUs from the RTX 2000 (and GTX 1600, Turing architecture) line. Apparently for the GTX 1000 cards (Pascal architecture) the Windows driver just resets the computer during boot if the BAR size has been changed, so GTX 1000 cards still can not enable ReBAR. The Linux driver does not crash, but does not pick up the new BAR size either.</p>

### Do I need to flash a new UEFI image on the motherboard, to enable ReBAR on the GPU ?
Yes, this is how it works for Turing GPUs (GTX 1600 / RTX 2000). It's ususally the video BIOS (vBIOS) that should enable ReBAR, but the vBIOS is digitally signed and can not be modified by end-users (is locked-down). The motherboard UEFI image can also be signed or have integrity checks, but in general it is thankfully not as locked down, and users and UEFI modders still have a way to modify it.

Currently the location of the GPU on the PCI bus has to be hard-coded right into the motherboard UEFI, and so does the associated PCI-to-PCI bridge. All hard-coded values are in the header file [`ReBarDxe/include/LocalPciGPU.h`](https://github.com/terminatorul/NvStrapsReBar/blob/master/ReBarDxe/include/LocalPciGPU.h), and all the values can be read from the CPU-Z .txt report file, so you can manually change them to match your system. Currently these settings are listed below, where all numeric values are examples only (for my computer):

```C++
#define TARGET_GPU_PCI_VENDOR_ID        0x10DEu
#define TARGET_GPU_PCI_DEVICE_ID        0x1E07u

#define TARGET_GPU_PCI_BUS              0x41u
#define TARGET_GPU_PCI_DEVICE           0x00u
#define TARGET_GPU_PCI_FUNCTION         0x00u

// PCIe config register offset 0x10
#define TARGET_GPU_BAR0_ADDRESS         UINT32_C(0x8200'0000)               // Should fall within memory range mapped by the bridge

#define TARGET_GPU_BAR1_SIZE_SELECTOR   _16G                                // Desired size for GPU BAR1, should cover the VRAM size

// Secondary bus of the bridge must match the GPU bus
// Check the output form CPU-Z .txt report

#define TARGET_BRIDGE_PCI_VENDOR_ID     0x1022u
#define TARGET_BRIDGE_PCI_DEVICE_ID     0x1453u

#define TARGET_BRIDGE_PCI_BUS           0x40u
#define TARGET_BRIDGE_PCI_DEVICE        0x03u
#define TARGET_BRIDGE_PCI_FUNCTION      0x01u

// Memory range and I/O port range (base + limit) mapped to bridge
// from CPU-Z .txt report of the bridge and GPU

// PCIe config register offset 0x20
#define TARGET_BRIDGE_MEM_BASE_LIMIT  UINT32_C(0x8300'8200)                 // Should cover the GPU BAR0

// PCIe config register offset 0x1C
#define TARGET_BRIDGE_IO_BASE_LIMIT   0x8181u
```

See https://github.com/xCuri0/ReBarUEFI/discussions/89#discussioncomment-7697768 for more step-by-step details.

Rebuild the project using the instructions below (that were slightly adapted from the original [ReBarUEFI](https://github.com/xCuri0/ReBARUEFI) project).

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

It maybe easier and more informative to post GPU-Z screenshots with the main GPU page + ReBAR page, and CPU-X with the CPU page and motherboard page screenshots. If you needed to apply more changes to make ReBAR work, please post about them as well.

## Building (Windows only)
* Download and install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/community/) from Microsoft. Be sure to select C/C++ Desktop Development option for installation.
* Download and install [Python 3](https://www.python.org/downloads/). Make sure you select the option to add python to PATH during installation.
* Download and install [git](https://git-scm.com/download/win)
* Download and install [CMake](https://cmake.org/download/)
* Downlaod and install [NASM](https://www.nasm.us/) and add the install directory to the PATH environment variable (in Windows Settings). The NASM installer does not show the option to add NASM to path at install time, so you will have to add it manually. NASM is by default installed at `%ProgramFiles%\NASM`, and you should add this directory to PATH. You can change your PATH variable from Windows Settings | System | About | Advanced Windows Settings | Environment Variables | System variables | PATH | Edit... | New...
* You can check all installed commands are available on PATH: open a new `cmd` console window, and run the following commands:
  ```
  python --version
  pip --version
  git --version
  cmake --version
  nasm --version
  ```
  If the output from any of the above commands says `'cmd-name' si not recognized as an internal or external command...` you need to check PATH environment variable again. Also make sure `python --version` outputs version 3 and not version 2.
* Install pefile module for python:
   - Use Win+R to open a cmd window and run `pip install pefile`
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
  - `If Not Defined VSCMD_VER "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"`
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
  If Not Defined VSCMD_VER "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
  ChDir "%UserProfile%\edk2"
  If Not Defined EDK_TOOLS_BIN edksetup.bat
  ChDir NvStrapsReBar\ReBarDxe
  python buildffs.py
  ```
  The NvStrapsReBar.ffs file will be found under the directory `%UserProfile%\edk2\Build\NvStrapsReBar\RELEASE_VS2019\X64\`.
* To build the Windows executable NvStrapsReBar.exe, run the following commands in the x86 Native Tools Command Prompt window
  ```
  If Not Defined VSCMD_VER "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
  ChDir "%UserProfile%\edk2"
  If Not Defined EDK_TOOLS_BIN edksetup.bat
  ChDir NvStrapsRebar\ReBarState
  If Not Exist build MkDir build
  ChDir build
  cmake ..
  cmake --build . --config Release
  ```
  The NvStrapsRebar.exe file will be found under the `%UserProfile%\edk2\NvStrapsRebar\ReBarState\build\Release\` subdirectory

## Updating UEFI image
The resulting `NvStrapsReBar.ffs` file needs to be included in the motherboard UEFI image (downloaded from the montherboard manufacturer), and the resulting image should be flashed onto the motherboard as if it were a new UEFI version for that board.
See the original project [ReBarUEFI](https://github.com/xCuri0/ReBarUEFI/) for the instructions to update motherboard UEFI. Replace "ReBarUEFI.ffs" with "NvStrapsReBar.ffs" where appropriate.

<p>So you will still have to check the README page from the original project: <ul><li><a href="https://github.com/xCuri0/ReBarUEFI">https://github.com/xCuri0/ReBarUEFI</a></li></ul> for all the details and instructions on working with the UEFI image, and patching it if necessary (for older motherboards and chipsets). </p>

## Enable ReBAR and choose BAR size
After flashing the motherboard with the new UEFI image, you need to enable "Above 4G decoding" and disable CSM in UEFI setup, and then run `NvStrapsReBar.exe` as Administrator.

`NvStrapsReBar.exe` prompts you with a small text-based menu. You can configure 2 value for the BAR size with this tool:
* GPU-side BAR size
* PCI BAR size

Both sizes must be right for Resizable BAR to work, but maybe some newer boards can already configure PCI BAR size as expected, so maybe there is a small chance you only need to set the GPU-side value for the BAR size. But you should try and experiment with both of them, as needed.

![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/c89a05d0-c5bf-4d2f-a1ba-90b7e951b44c)

Most people should choose the first menu option and press `E` to Enable auto-settings BAR size for Turing GPUs. Depending on your board, you may need to also input `P` at the menu prompt, to choose Target PCI BAR size, and select value 64 (for the option to configure PCI BAR for selected GPUs only). Befor quitting the menu, input `S` to save the changes you made to the EFI variable store, for the UEFI DXE driver to read them.

If you choose a GPU BAR size of 8 GiB for example, and a Target PCI BAR size of 4 GiB, you will get a 4 GiB BAR.

For older boards without ReBAR support from the manufacturer, you can select other values for Target PCI BAR size, to also configure other GPUs for example. Or to limit the BAR size to smaller values even if the GPU supports higher values. Depending on the motherboard UEFI, for some boards you may need to use lower values, to limit BAR size to 4 GB or 2GB for example.

## Using large BAR sizes
Remember you need to use the [Profile Inspector](https://github.com/Orbmu2k/nvidiaProfileInspector) to enable ReBAR per-application, and if neeeded also globally. There appears to be a fake site for the Profile Inspector, so always downloaded it from github, or use the link above.

