# v3x4
Intel(R) Xeon(R) Processor v3 (Haswell-E/EP) Full Turbo Boost DXE driver

In Windows, to compile, you will need:

1) UDK2015: http://www.tianocore.org/udk/udk2015/

2) Any C-compiler that is supported by UDK2015. Currently verified working with Visual Studio 2015.

3) In file [UDK2015]\BaseTools\Conf\target.txt change next parameters to:

ACTIVE_PLATFORM = MdeModulePkg/MdeModulePkg.dsc
TARGET = RELEASE
TARGET_ARCH = X64
TOOL_CHAIN_TAG = VS2015x86 (or VS2013x86, etc.)

4) open command prompt (admin0, go to folder [UDK2015]\BaseTools

5) execute edksetup.bat

6) execute build

If it prints - Done - all is fine and UDK2015 is functioning properly...

7) unpack source into [UDK2015]\BaseTools\MdeModulePkg\v3x4

8) in file [UDK2015]\BaseTools\MdeModulePkg\MdeModulePkg.dsc, in section [Components], add string MdeModulePkg/v3x4/v3x4.inf

9) execute build again

10) wait for - Done - and take compiled EFI DVX driver from
[UDK2015]\BaseTools\Build\MdeModule\RELEASE_VS2013x86\X64\MdeModulePkg\v3x4\v3x4\OUTPUT\v3x4.efi
