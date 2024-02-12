<h1 align="center">NvStrapsReBar</h1>
<p>UEFI driver to enable and test Resizable BAR on Turing graphics cards (GTX 1600, RTX 2000). Pre-Pascal cards might also work.</p>
<p>This is a copy of the rather popular <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> DXE driver. <a href="https://github.com/xCuri0/ReBARUEFI">ReBarUEFI</a> enables Resizable BAR for older motherboards and chipsets without ReBAR support from the manufacturer. NvStrapsReBar was created to test Resizable BAR support for GPUs from the RTX 2000 (and GTX 1600, Turing architecture) line. Apparently for the GTX 1000 cards (Pascal architecture) the Windows driver just resets the computer during boot if the BAR size has been changed, so GTX 1000 cards still can not enable ReBAR. The Linux driver does not crash, but does not pick up the new BAR size either.</p>

### Do I need to flash a new UEFI image on the motherboard, to enable ReBAR on the GPU ?
Yes, this is how it works for Turing GPUs (GTX 1600 / RTX 2000).

It's ususally the video BIOS (vBIOS) that should enable ReBAR, but the vBIOS is digitally signed and can not be modified by modders and end-users (is locked-down). The motherboard UEFI image can also be signed or have integrity checks, but in general it is thankfully not as locked down, and users and UEFI modders often still have a way to modify it.

For older boards without ReBAR, adding ReBAR functionality depends on the Above 4G Decoding option in your UEFI setup (if you have it), which must be turned on in advance, and CSM must be disabled. If you accidentaly turn off 4G decoding and are unable to boot, you need to clear CMOS. Do not use the Clear CMOS button that is present on some motherboards, instead short the pin headers, or remove the battery. The button may still keep the date and time, and the date needs to be reset to recover the board. If you can enter UEFI Setup, you can also manually set back the year to a value before 2024, reboot, and restore it after.

#### Warning:
Some users report BSOD or crash when resuming from suspend, which can be even worse for laptop users trying to stay mobile. Developing a boot script for the DXE driver might address the issue, but the feature is not implemented and there is currently no fix for this issue.

### Usage
Build the project using the instructions below (that were slightly adapted from the original [ReBarUEFI](https://github.com/xCuri0/ReBARUEFI) project). This should produce two files:
* `NvStrapsReBar.ffs` UEFI DXE driver
* `NvStrapsReBar.exe` Windows executable

After building you need to go through a number of steps:
* update the motherbord UEFI image to add the new `NvStrapsReBar.ffs` driver (see below)
* enable ReBAR, enable "Above 4G Decoding" (if you have the option) and disable CSM in UEFI Setup
* run `NvStrapsReBar.exe` as Administrator to enable the new BAR size, by following the text-mode menus. If you have a recent motherboard, you only need to input `E` to Enable ReBAR for Turing GPUS, then input `S` to save the new driver configuration to EFI variable. For older motherboards without ReBAR, you also need to input `P` to set BAR size on the PCI side (motherboard side).
* reboot after saving the menu options.
* for older motherboards without ReBAR, if you want to load default UEFI settings again, or disable Above 4G Decoding / enable CSM, you need to  disable ReBAR first in `NvStrapsReBar.exe`. Or you can manually set back the current year in UEFI Setup.

![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/09756f17-804a-40e8-86b7-306014061732)


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

## Building (Windows only)
* Download and install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/community/) from Microsoft. Be sure to select C/C++ Desktop Development option for installation.
* Download and install [Python 3](https://www.python.org/downloads/). Make sure you select the option to add python to PATH during installation.
* Download and install [git](https://git-scm.com/download/win)
* Download and install [CMake](https://cmake.org/download/)
* Download and install [NASM](https://www.nasm.us/), and update environment variables `NASM_PREFIX` and `PATH` with the chosen install location. By default it is %ProgramFiles%\NASM. Set NASM_PREFIX environment variable to `%ProgramFiles%\NASM\` (NOTE the backslash character `\` at the end of the path), and also add %ProgramFiles%\NASM to the PATH environment variable (in Windows Settings). You can change your NASM_PREFIX and PATH variable from Start Menu | Windows Settings | System | About | Advanced Windows Settings | Environment Variables | System variables | PATH | Edit... | New...
* You can check all installed commands are available on PATH: open a new `cmd` console window, and run the following commands:
  ```
  python --version
  pip --version
  git --version
  nasm --version
  cmake --version
  ```
  If the output from any of the above commands says `'cmd-name' is not recognized as an internal or external command...` you need to check PATH environment variable again. Also make sure `python --version` outputs version 3 and not version 2.
* Install pefile module for python:
   - Use Win+R to open a `cmd` window and run `pip install pefile`
* Get `edk2` framework from github. In a cmd window run:
  ```
  git -C "%UserProfile%" clone https://github.com/tianocore/edk2.git
  git -C "%UserProfile%\edk2" submodule update --init --jobs "%Number_Of_Processors%"
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
  cmake .. && cmake --build . --config Release
  ```
  The NvStrapsRebar.exe file will be found under the `%UserProfile%\edk2\NvStrapsRebar\ReBarState\build\Release\` subdirectory

## Updating UEFI image
The resulting `NvStrapsReBar.ffs` file needs to be included in the motherboard UEFI image (downloaded from the montherboard manufacturer), and the resulting image should be flashed onto the motherboard as if it were a new UEFI version for that board.
See the original project [ReBarUEFI](https://github.com/xCuri0/ReBarUEFI/) for the instructions to update motherboard UEFI. Replace "ReBarUEFI.ffs" with "NvStrapsReBar.ffs" where appropriate.

<p>So you will still have to check the README page from the original project: <ul><li><a href="https://github.com/xCuri0/ReBarUEFI">https://github.com/xCuri0/ReBarUEFI</a></li></ul> for all the details and instructions on working with the UEFI image, and patching it if necessary (for older motherboards and chipsets). </p>

## Enable ReBAR and choose BAR size
After flashing the motherboard with the new UEFI image, you need to enable ReBAR, enable "Above 4G Decoding" (if you have the option) and disable CSM in UEFI setup, and then run `NvStrapsReBar.exe` as Administrator.

For older motherboard without ReBAR support, enablging ReBAR depends on Above 4G Decoding, so if you accidentaly turn it off and can not POST, you need to clear CMOS. Do not use the Clear CMOS button present on some motherboards, as it may still keep the current date and time. The current date has to be reset to recover the board. So instead of the Clear CMOS button, you should short the Clear CMOS pin headers (with a screwdriver that is metallic, and the metal is not painted), or by removing the battery for 1 to 5 minutes. If you can enter UEFI Setup, you can also manually set back the year to a value before 2024, reboot, and restore the year after.

`NvStrapsReBar.exe` prompts you with a small text-based menu. You can configure 2 value for the BAR size with this tool:
* GPU-side BAR size
* PCI BAR size (for older motherboards without ReBAR)

Both sizes must be right for Resizable BAR to work, but newer boards can configure PCI BAR size as expected, so you only need to set the GPU-side value for the BAR size. If not, you should try and experiment with both of them, as needed.

![image](https://github.com/terminatorul/NvStrapsReBar/assets/378924/fc432819-6710-43da-829f-41c2119b89d7)


Most people should choose the first menu option and press `E` to Enable auto-settings BAR size for Turing GPUs. Depending on your board, you may need to also input `P` at the menu prompt, to choose Target PCI BAR size, and select value 64 (for the option to configure PCI BAR for selected GPUs only). Before quitting the menu, input `S` to save the changes you made to the EFI variable store, for the UEFI DXE driver to read them.

If you choose a GPU BAR size of 8 GiB for example, and a Target PCI BAR size of 4 GiB, you will get a 4 GiB BAR.

For older boards without ReBAR support from the manufacturer, you can select other values for Target PCI BAR size, to also configure other GPUs for example. Or to limit the BAR size to smaller values even if the GPU supports higher values. Depending on the motherboard UEFI, for some boards you may need to use lower values, to limit BAR size to 4 GB or 2GB for example. Even a 2 GB BAR size still gives you the benefits of Resizable BAR in most titles, and NVIDIA tends to use 1.5 GB as the default size in the Profile Inspector. There are exceptions to this 'though (for some titles that can still see improvements with the higher BAR sizes).

## Using large BAR sizes
Remember you need to use the [Profile Inspector](https://github.com/Orbmu2k/nvidiaProfileInspector) to enable ReBAR per-application, and if neeeded also globally. There appears to be a fake site for the Profile Inspector, so always downloaded it from github, or use the link above.

