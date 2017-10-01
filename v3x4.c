
// ***************************************************************************************************************
// ** Intel(R) Xeon(R) Processor v3 (Haswell-E/EP) Full Turbo Boost DXE driver
// ***************************************************************************************************************

#include <PiPei.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Protocol/MpService.h>

// negative dynamic voltage offsets for ALL domains
#define		_no_offset							0		// no change to factory voltage setting
#define		_neg_1_mV							0xFFE00000	// -1mV
#define		_neg_2_mV							0xFFC00000	// -2mV
#define		_neg_3_mV							0xFFA00000	// -3mV
#define		_neg_4_mV							0xFF800000	// -4mV
#define		_neg_5_mV							0xFF600000	// -5mV
#define		_neg_6_mV							0xFF400000	// -6mV
#define		_neg_7_mV							0xFF200000	// -7mV
#define		_neg_8_mV							0xFF000000	// -8mV
#define		_neg_9_mV							0xFEE00000	// -9mV
#define		_neg_10_mV							0xFEC00000	// -10mV
#define		_neg_11_mV							0xFEA00000	// -11mV
#define		_neg_12_mV							0xFE800000	// -12mV
#define		_neg_13_mV							0xFE600000	// -13mV
#define		_neg_14_mV							0xFE400000	// -14mV
#define		_neg_15_mV							0xFE200000	// -15mV
#define		_neg_16_mV							0xFE000000	// -16mV
#define		_neg_17_mV							0xFDE00000	// -17mV
#define		_neg_18_mV							0xFDC00000	// -18mV
#define		_neg_19_mV							0xFDA00000	// -19mV
#define		_neg_20_mV							0xFD800000	// -20mV
#define		_neg_21_mV							0xFD400000	// -21mV
#define		_neg_22_mV							0xFD200000	// -22mV
#define		_neg_23_mV							0xFD000000	// -23mV
#define		_neg_24_mV							0xFCE00000	// -24mV
#define		_neg_25_mV							0xFCC00000	// -25mV
#define		_neg_26_mV							0xFCA00000	// -26mV
#define		_neg_27_mV							0xFC800000	// -27mV
#define		_neg_28_mV							0xFC600000	// -28mV
#define		_neg_29_mV							0xFC400000	// -29mV
#define		_neg_30_mV							0xFC200000	// -30mV
#define		_neg_31_mV							0xFC000000	// -31mV
#define		_neg_32_mV							0xFBE00000	// -32mV
#define		_neg_33_mV							0xFBC00000	// -33mV
#define		_neg_34_mV							0xFBA00000	// -34mV
#define		_neg_35_mV							0xFB800000	// -35mV
#define		_neg_36_mV							0xFB600000	// -36mV
#define		_neg_37_mV							0xFB400000	// -37mV
#define		_neg_38_mV							0xFB200000	// -38mV
#define		_neg_39_mV							0xFB000000	// -39mV
#define		_neg_40_mV							0xFAE00000	// -40mV
#define		_neg_41_mV							0xFAC00000	// -41mV
#define		_neg_42_mV							0xFAA00000	// -42mV
#define		_neg_43_mV							0xFA800000	// -43mV
#define		_neg_44_mV							0xFA600000	// -44mV
#define		_neg_45_mV							0xFA400000	// -45mV
#define		_neg_46_mV							0xFA200000	// -46mV
#define		_neg_47_mV							0xFA000000	// -47mV
#define		_neg_48_mV							0xF9E00000	// -48mV
#define		_neg_49_mV							0xF9C00000	// -49mV
#define		_neg_50_mV							0xF9A00000	// -50mV
#define		_neg_51_mV							0xF9800000	// -51mV
#define		_neg_52_mV							0xF9600000	// -52mV
#define		_neg_53_mV							0xF9400000	// -53mV
#define		_neg_54_mV							0xF9200000	// -54mV
#define		_neg_55_mV							0xF9000000	// -55mV
#define		_neg_56_mV							0xF8E00000	// -56mV
#define		_neg_57_mV							0xF8C00000	// -57mV
#define		_neg_58_mV							0xF8A00000	// -58mV
#define		_neg_59_mV							0xF8800000	// -59mV
#define		_neg_60_mV							0xF8600000	// -60mV
#define		_neg_61_mV							0xF8400000	// -61mV
#define		_neg_62_mV							0xF8200000	// -62mV
#define		_neg_63_mV							0xF7E00000	// -63mV
#define		_neg_64_mV							0xF7C00000	// -64mV
#define		_neg_65_mV							0xF7A00000	// -65mV
#define		_neg_66_mV							0xF7800000	// -66mV
#define		_neg_67_mV							0xF7600000	// -67mV
#define		_neg_68_mV							0xF7400000	// -68mV
#define		_neg_69_mV							0xF7200000	// -69mV
#define		_neg_70_mV							0xF7000000	// -70mV
#define		_neg_71_mV							0xF6E00000	// -71mV
#define		_neg_72_mV							0xF6C00000	// -72mV
#define		_neg_73_mV							0xF6A00000	// -73mV
#define		_neg_74_mV							0xF6800000	// -74mV (same as -75mV)
#define		_neg_75_mV							0xF6800000	// -75mV (same as -74mV)
#define		_neg_76_mV							0xF6400000	// -76mV
#define		_neg_77_mV							0xF6200000	// -77mV
#define		_neg_78_mV							0xF6000000	// -78mV
#define		_neg_79_mV							0xF5E00000	// -79mV
#define		_neg_80_mV							0xF5C00000	// -80mV
#define		_neg_81_mV							0xF5A00000	// -81mV
#define		_neg_82_mV							0xF5800000	// -82mV
#define		_neg_83_mV							0xF5600000	// -83mV
#define		_neg_84_mV							0xF5400000	// -84mV
#define		_neg_85_mV							0xF5200000	// -85mV
#define		_neg_86_mV							0xF5000000	// -86mV
#define		_neg_87_mV							0xF4E00000	// -87mV
#define		_neg_88_mV							0xF0C00000	// -88mV
#define		_neg_89_mV							0xF4A00000	// -89mV
#define		_neg_90_mV							0xF4800000	// -90mV
#define		_neg_91_mV							0xF4600000	// -91mV
#define		_neg_92_mV							0xF4400000	// -92mV
#define		_neg_93_mV							0xF4200000	// -93mV
#define		_neg_94_mV							0xF4000000	// -94mV
#define		_neg_95_mV							0xF3E00000	// -95mV
#define		_neg_96_mV							0xF3C00000	// -96mV
#define		_neg_97_mV							0xF3A00000	// -97mV
#define		_neg_98_mV							0xF3800000	// -98mV
#define		_neg_99_mV							0xF3600000	// -99mV
#define		_neg_100_mV							0xF3400000	// -100mV
#define		_neg_110_mV							0xF1E00000	// -110mV
#define		_neg_120_mV							0xF0A00000	// -120mV
#define		_neg_130_mV							0xEF600000	// -130mV
#define		_neg_140_mV							0xEE200000	// -140mV
#define		_neg_150_mV							0xECC00000	// -150mV

// Intel CPU Model Specific Registers
#define		MSR_IA32_BIOS_SIGN_ID				0x08B
#define		MSR_PLATFORM_INFO				0x0CE
#define		MSR_OC_MAILBOX					0x150				// no documentation available in the public domain
#define		MSR_FLEX_RATIO					0x194
#define		MSR_TURBO_RATIO_LIMIT				0x1AD
#define		MSR_TURBO_RATIO_LIMIT1 				0x1AE
#define		MSR_TURBO_RATIO_LIMIT2 				0x1AF
#define		UNCORE_RATIO_LIMIT				0x620

// toolbox for OC Mailbox commands - experimental
#define		OC_MAILBOX_COMMAND_EXEC				0x8000000000000000ull

#define		OC_MAILBOX_GET_CPU_CAPS				0x0000000100000000ull
#define		OC_MAILBOX_GET_TURBO_RATIOS			0x0000000200000000ull
#define		OC_MAILBOX_GET_VID_PARAMS			0x0000001000000000ull
#define		OC_MAILBOX_SET_VID_PARAMS			0x0000001100000000ull
#define		OC_MAILBOX_GET_SVID				0x0000001200000000ull
#define		OC_MAILBOX_SET_SVID				0x0000001300000000ull
#define		OC_MAILBOX_GET_FIVR				0x0000001400000000ull
#define		OC_MAILBOX_SET_FIVR				0x0000001500000000ull

#define		OC_MAILBOX_DOMAIN_0				0x0000000000000000ull		// Core domain
#define		OC_MAILBOX_DOMAIN_2				0x0000020000000000ull		// CLR (CBo/LLC/Ring) domain
#define		OC_MAILBOX_DOMAIN_3				0x0000030000000000ull		// System Agent domain

#define		OC_MAILBOX_RESPONSE_MASK			0x000000FF00000000ull
#define		OC_MAILBOX_RESPONSE_SUCCESS			0

// general definitions
#define		BUS_FREQUENCY					100				// placeholder; TO DO: add bus frequency sensing

#define		CPUID_VERSION_INFO				0x00000001
#define		CPUID_BRAND_STRING_BASE				0x80000002
#define		CPUID_BRAND_STRING_LEN				48

#define		MSR_FLEX_RATIO_OC_LOCK_BIT			0x0000000000100000ull
#define		MSR_TURBO_RATIO_SEMAPHORE_BIT			0x8000000000000000ull

#define		MAX_PACKAGE_COUNT				4				// maximum number of packages supported by the build

// ***EDIT HERE*** Build options ***EDIT HERE***
#define		_VERBOSE_OUTPUT_								// more verbose console outputs, comment out to disable

#define		_SET_OVERCLOCKING_LOCK_								// set MSR 0x194[0x20] to prevent any further changes, comment out to disable

#define		_DOMAIN0_CLOCKING_ENABLED_							// SAFETY FLAG: enable Core ratios to maximum turbo (all-core turbo) for maximum performance, comment out to disable
#define		_DOMAIN2_CLOCKING_ENABLED_							// SAFETY FLAG: enable set static Uncore ratio to maximum performance, comment out to disble

#define		_DOMAIN0_VOLTAGE_ENABLED_							// SAFETY FLAG: enable programming Core voltages offset, comment out to disable
#define		_DOMAIN2_VOLTAGE_ENABLED_							// SAFETY FLAG: enable programming CLR voltage offset, comment out to disable
#define		_DOMAIN3_VOLTAGE_ENABLED_							// SAFETY FLAG: enable programming SA voltage offset, comment out to disable

#define		TARGET_CPU_CPUID_SIGN				0x000306F2			// CPUID for Haswell-E/EP final QS/production silicon is 0x306F2 (Extended Family: 3F, Model: 6, Stepping: 2)
																			// set 0xFFFFFFFF to override and allow to attempt to run for all CPUs (warning: can cause system freeze at POST)

// ***EDIT HERE*** CPU dynamic voltage offsets by domain in order of Package ***EDIT HERE***

// Core dynamic voltage offsets
const UINT32 kcpu_domain_0_voltage_offset[MAX_PACKAGE_COUNT] \
	= { _neg_90_mV, _neg_90_mV, _no_offset, _no_offset };					

// CLR (CBo/LLC/Ring) dynamic voltage offsets
const UINT32 kcpu_domain_2_voltage_offset[MAX_PACKAGE_COUNT] \
	= { _neg_50_mV, _neg_50_mV, _no_offset, _no_offset };					

// System Agent dynamic voltage offsets
const UINT32 kcpu_domain_3_voltage_offset[MAX_PACKAGE_COUNT] \
	= { _neg_50_mV, _neg_50_mV, _no_offset, _no_offset };  

// System Topology object
typedef struct _SYSTEM_TOPOLOGY_OBJECT {
	UINTN						Packages;				// number of physical processor packages
	UINTN						LogicalProcessors;			// total number of logical processors
	UINTN						EnabledLogicalProcessors;		// total number of enabled logical processors
	UINTN						Cores[MAX_PACKAGE_COUNT];		// number cores in the package
	UINTN						APICID[MAX_PACKAGE_COUNT];		// logical processor number for programming/unlocking
	UINTN						HyperThreading;				// 0 = HyperThreading disabled, 1 = HyperThreading enabled
} SYSTEM_TOPOLOGY_OBJECT, *PSYSTEM_TOPOLOGY_OBJECT;

// Processor Package object
typedef struct _PROCESSOR_PACKAGE_OBJECT {
	UINTN						MinFreqModeRatio;			// Minimum Frequency Mode (MFM) "Low Power Mode" (LPM)
	UINTN						LowFreqModeRatio;			// Low Frequency Mode (LFM), minimum non-turbo Core multiplier
	UINTN						HighFreqModeRatio;			// High Frequecy Mode (HFM), maximum non-turbo Core multiplier
	UINTN						MaxTurboRatio;				// maximum (1C) turbo Core multiplier
	UINTN						MaxUncoreRatio;				// maximum Uncore multiplier
	UINT32						ProcessorCPUID;				// CPUID
	CHAR16						BrandNameString[100];			// Processor Brand Name string (16-bit UNICODE)
} PROCESSOR_PACKAGE_OBJECT, *PPROCESSOR_PACKAGE_OBJECT;

// driver specific device extension object
typedef struct _V3DRIVER_DEVICE_EXTENTION {
	PSYSTEM_TOPOLOGY_OBJECT		SystemTopology;						// System Topology object
	PPROCESSOR_PACKAGE_OBJECT	ProcessorPackage;					// array of Processor Package objects
	UINTN						BootstrapProcessor;			// bootstrap processor assignment at driver entry
	UINTN						NextPackage;				// package number of next package to be programmed/unlocked
} V3DRIVER_DEVICE_EXTENTION, *PV3DRIVER_DEVICE_EXTENTION;

// global variables
EFI_MP_SERVICES_PROTOCOL		*MpServicesProtocol;					// ptr to MP Services Protocol
V3DRIVER_DEVICE_EXTENTION		*DeviceExtension;					// ptr to device extension

// function prototypes
EFI_STATUS	EFIAPI InitSystem(IN OUT PSYSTEM_TOPOLOGY_OBJECT *SystemTopologyObject);
EFI_STATUS	EFIAPI EnumProcessors(IN OUT PPROCESSOR_PACKAGE_OBJECT *ProcessorPackageObject);
VOID		EFIAPI UnlockProcessor(IN OUT VOID *Buffer);
BOOLEAN		EFIAPI IsMicrocodePresent(VOID);

// driver main
EFI_STATUS
EFIAPI
EFIDriverEntry(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
)
{
	EFI_STATUS status;

	// driver init
	Print(
		L"Intel(R) Xeon(R) Processor v3 (Haswell-E/EP) Full Turbo Boost DXE driver\r\n\0"
		);

	// get handle to MP Services Protocol
	EFI_GUID efi_mp_service_protocol_guid = EFI_MP_SERVICES_PROTOCOL_GUID;
	
	status = SystemTable->BootServices->LocateProtocol(
		&efi_mp_service_protocol_guid,
		NULL,
		(VOID**)&MpServicesProtocol
		);

	if (EFI_ERROR(status)) {
		Print(
			L"Error: Unable to locate EFI MP Services Protocol (%r)\r\n\0",
			status
			);
		
		goto DriverExit; 
	}

	// verify no microcode patch is loaded, exit if detected
	if (IsMicrocodePresent() == TRUE) {
		Print(
			L"Error: Processor microcode update revision detected\r\n\0"
			);

		goto DriverExit;
	}

	// allocate pool memory for device extension
	DeviceExtension = (PV3DRIVER_DEVICE_EXTENTION)AllocatePool(
		sizeof(V3DRIVER_DEVICE_EXTENTION)
		);

	if (!DeviceExtension) {
		goto DriverExit;
	}

	// zero it
	SetMem(
		DeviceExtension,
		sizeof(V3DRIVER_DEVICE_EXTENTION),
		0
		);
	
	// get initial BSP
	status = MpServicesProtocol->WhoAmI(
		MpServicesProtocol,
		&DeviceExtension->BootstrapProcessor
		);

	if (EFI_ERROR(status)) {
		Print(
			L"Error: Failed to get current bootstrap processor (%r)\r\n\0",
			status
			);

		goto DriverExit;
	}

	// initialize System
	if (EFI_ERROR(
		InitSystem(
			&DeviceExtension->SystemTopology
		))) {
		goto DriverExit;
	}

	// enumerate Processor Packages
	if (EFI_ERROR(
		EnumProcessors(
			&DeviceExtension->ProcessorPackage
		))) {
		goto DriverExit;
	}

	for (UINTN package_index = 0; package_index < DeviceExtension->SystemTopology->Packages; package_index++) {
		// program CPU using current AP if BSP already CPU0 (likely)
		if (DeviceExtension->BootstrapProcessor == DeviceExtension->SystemTopology->APICID[DeviceExtension->NextPackage]) {
			UnlockProcessor(NULL);			
		}
		else
		{
			// dispatch AP to program CPUx
			status = MpServicesProtocol->StartupThisAP(
				MpServicesProtocol,
				UnlockProcessor,
				DeviceExtension->SystemTopology->APICID[DeviceExtension->NextPackage],
				NULL,
				100000,
				NULL,
				NULL
				);

			if (EFI_ERROR(status)) {
				Print(
					L"Warning: Failed to startup programming AP on CPU%d (%r)\r\n\0",
					DeviceExtension->NextPackage,
					status
					);
			}
		}
		
		// next package in sequence
		DeviceExtension->NextPackage++;
	}
	
DriverExit:

	// always return success
	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitSystem(
	IN OUT PSYSTEM_TOPOLOGY_OBJECT *SystemTopologyObject
)
{
	// allocate memory pool space
	PSYSTEM_TOPOLOGY_OBJECT SystemTopology = (PSYSTEM_TOPOLOGY_OBJECT)AllocatePool(
		sizeof(SYSTEM_TOPOLOGY_OBJECT)
		);

	if (!SystemTopology) {
		return EFI_OUT_OF_RESOURCES; 
	}

	// zero it
	SetMem(
		SystemTopology,
		sizeof(SYSTEM_TOPOLOGY_OBJECT),
		0
		);

	// set initial values
	SystemTopology->Packages = 1;
		
	// get number of logical processors
	EFI_STATUS status = MpServicesProtocol->GetNumberOfProcessors(
		MpServicesProtocol,
		&SystemTopology->LogicalProcessors,
		&SystemTopology->EnabledLogicalProcessors
		);
	
	if (EFI_ERROR(status)) {
		return status;
	}
			
	// get APICID for first logical processor in each detected package, get number of Cores per package
	EFI_PROCESSOR_INFORMATION processor_info;
	UINTN core_count = 0;
	
	for (UINTN processor_index = 0; processor_index < SystemTopology->LogicalProcessors; processor_index++)
	{
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			processor_index,
			&processor_info
			);

		// detect if HyperThreading enabled
		if (processor_info.Location.Thread == 1) {
			SystemTopology->HyperThreading = 1;
		}
				
		// count up the number of cores per package on a per-package basis
		core_count++;

		// package ID changes -> new package found
		if (processor_info.Location.Package > (SystemTopology->Packages - 1)) {
			SystemTopology->APICID[processor_info.Location.Package - 1] = (processor_index - core_count) + 1;
			
			SystemTopology->Cores[processor_info.Location.Package - 1] = core_count - 1;
			
			// count up the number of packages
			SystemTopology->Packages++;
			
			// rest core count to start counting again
			core_count = 0;
		}

		// last logical processor, no more new package detections
		if (processor_index == (SystemTopology->LogicalProcessors - 1)) {
			
			SystemTopology->APICID[SystemTopology->Packages - 1] = processor_index - core_count;
			
			SystemTopology->Cores[SystemTopology->Packages - 1] = core_count + 1;
		}
	}
	
	// display results to console
	#ifdef _VERBOSE_OUTPUT_

	Print(
		L"\r\n\0"
		);

	#endif // _VERBOSE_OUTPUT_
	
	Print(L"Processor Packages: %d, Logical Processors: %d (%dC/%dT)\r\n\0",
		SystemTopology->Packages, SystemTopology->LogicalProcessors,
		SystemTopology->LogicalProcessors / (SystemTopology->HyperThreading + 1),
		SystemTopology->LogicalProcessors
		);

	#ifdef _VERBOSE_OUTPUT_

	for (UINTN package_count = 0; package_count < SystemTopology->Packages; package_count++)
	{
		Print(L" -- Processor %d (CPU%d) Cores: %d, Threads: %d, APIC ID: %02xh\r\n\0",
			package_count,
			package_count,
			SystemTopology->Cores[package_count] / (SystemTopology->HyperThreading + 1),
			SystemTopology->Cores[package_count], 
			SystemTopology->APICID[package_count]
			);
	}

	Print(
		L"\r\n\0"
		);
		
	#endif // _VERBOSE_OUTPUT_

	*SystemTopologyObject = SystemTopology;
	
	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EnumProcessors(
	IN OUT PPROCESSOR_PACKAGE_OBJECT *ProcessorPackageObject
)
{
	UINT64 msr_ret;
	
	// allocate memory pool space
	PPROCESSOR_PACKAGE_OBJECT ProcessorPackage = (PPROCESSOR_PACKAGE_OBJECT)AllocatePool(
		DeviceExtension->SystemTopology->Packages * sizeof(PROCESSOR_PACKAGE_OBJECT)
		);
	
	if (!ProcessorPackage) {
		return EFI_OUT_OF_RESOURCES;
	}

	// zero it
	SetMem(
		ProcessorPackage, 
		sizeof(PROCESSOR_PACKAGE_OBJECT),
		0
		);
	
	for (UINTN processor_index = 0; processor_index < DeviceExtension->SystemTopology->Packages; processor_index++)
	{			
		// get processor CPUID
		AsmCpuid(
			CPUID_VERSION_INFO,
			&ProcessorPackage[processor_index].ProcessorCPUID,
			NULL,
			NULL,
			NULL
			);

		// throw error if does not match target CPUID (unless override set)
		if ((ProcessorPackage[processor_index].ProcessorCPUID != TARGET_CPU_CPUID_SIGN) && \
			(TARGET_CPU_CPUID_SIGN != 0xFFFFFFFF)) {
			Print(
				L"Error: CPU%d Processor CPUID (0x%x) not equal to 0x%x\r\n\0",
				processor_index,
				ProcessorPackage[processor_index].ProcessorCPUID,
				TARGET_CPU_CPUID_SIGN
				);

			return EFI_ABORTED;
		}

		// build Processor Brand Name string (8-bit ASCII)		
		CHAR8 processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1];
		UINT32 cpuid_string[4];
		UINTN k = 0;

		for (UINTN i = 0; i < 3; i++) {
	
			AsmCpuid(
				(UINT32)(CPUID_BRAND_STRING_BASE + i),
				&cpuid_string[0],
				&cpuid_string[1],
				&cpuid_string[2],
				&cpuid_string[3]
				);

			for (UINTN j = 0; j < 4; j++) {
				CopyMem(
					processor_brand_string_buffer + k,
					&cpuid_string[j],
					4
					);

				k = k + 4;
			}
		}
		
		processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

		// convert ASCII to Unicode
		AsciiStrToUnicodeStrS(
			processor_brand_string_buffer,
			ProcessorPackage[processor_index].BrandNameString,
			sizeof(ProcessorPackage[processor_index].BrandNameString)
			);

		// get LPM/LFM/HFM (Min/Low/High) Core ratios
		msr_ret = AsmReadMsr64(
			MSR_PLATFORM_INFO
			);

		ProcessorPackage[processor_index].MinFreqModeRatio = ((msr_ret & 0x00FF000000000000ull) >> 48);
		
		ProcessorPackage[processor_index].LowFreqModeRatio = ((msr_ret & 0x0000FF0000000000ull) >> 40);
		
		ProcessorPackage[processor_index].HighFreqModeRatio = ((msr_ret & 0x000000000000FF00ull) >> 8);

		// get Core Turbo ratio via OC Mailbox
		AsmWriteMsr64(
			MSR_OC_MAILBOX,
			OC_MAILBOX_GET_CPU_CAPS | OC_MAILBOX_DOMAIN_0 | OC_MAILBOX_COMMAND_EXEC
			);

		msr_ret = AsmReadMsr64(
			MSR_OC_MAILBOX
			);

		if ((msr_ret & OC_MAILBOX_RESPONSE_MASK) != OC_MAILBOX_RESPONSE_SUCCESS) {
			Print(
				L"Error: CPU%d OC Mailbox failure getting maximum Core turbo multiplier\r\n\0",
				processor_index
				);
			
			return EFI_ABORTED;
		}

		ProcessorPackage[processor_index].MaxTurboRatio = msr_ret & 0x00000000000000FFull;

		// get maximum Uncore ratio via OC Mailbox
		AsmWriteMsr64(
			MSR_OC_MAILBOX,
			OC_MAILBOX_GET_CPU_CAPS | OC_MAILBOX_DOMAIN_2 | OC_MAILBOX_COMMAND_EXEC
			);

		msr_ret = AsmReadMsr64(
			MSR_OC_MAILBOX
			);

		if ((msr_ret & OC_MAILBOX_RESPONSE_MASK) != OC_MAILBOX_RESPONSE_SUCCESS) {
			Print(
				L"Error: CPU%d OC Mailbox failure getting maximum Uncore multiplier\r\n\0",
				processor_index
				);
			
			return EFI_ABORTED;
		}

		ProcessorPackage[processor_index].MaxUncoreRatio = msr_ret & 0x00000000000000FFull;
		
		// display results to console
		Print(
			L"Detected CPU%d: %s\r\n\0",
			processor_index,
			ProcessorPackage[processor_index].BrandNameString
			);

		#ifdef _VERBOSE_OUTPUT_

		Print(
			L" -- CPU%d Min Frequency Mode (MFM): %d MHz (= %d x %d MHz)\r\n\0",
			processor_index,
			ProcessorPackage[processor_index].MinFreqModeRatio * BUS_FREQUENCY,
			ProcessorPackage[processor_index].MinFreqModeRatio,
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d Low Frequency Mode (LFM): %d MHz (= %d x %d MHz)\r\n\0", 
			processor_index,
			ProcessorPackage[processor_index].LowFreqModeRatio * BUS_FREQUENCY,
			ProcessorPackage[processor_index].LowFreqModeRatio,
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d High Frequency Mode (HFM): %d MHz (= %d x %d MHz)\r\n\0",
			processor_index,
			ProcessorPackage[processor_index].HighFreqModeRatio * BUS_FREQUENCY,
			ProcessorPackage[processor_index].HighFreqModeRatio,
			BUS_FREQUENCY
			);

		Print(L" -- CPU%d Max Core Turbo Frequency: %d MHz (= %d x %d MHz)\r\n\0",
			processor_index,
			ProcessorPackage[processor_index].MaxTurboRatio * BUS_FREQUENCY,
			ProcessorPackage[processor_index].MaxTurboRatio, 
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d Max Uncore Frequency: %d MHz (= %d x %d MHz)\r\n\0",
			processor_index,
			ProcessorPackage[processor_index].MaxUncoreRatio * BUS_FREQUENCY,
			ProcessorPackage[processor_index].MaxUncoreRatio,
			BUS_FREQUENCY
			);
		
		Print(
			L"\r\n\0"
			);

		#endif // _VERBOSE_OUTPUT_		
	}
	
	*ProcessorPackageObject = ProcessorPackage;
	
	return EFI_SUCCESS;
}

VOID
EFIAPI
UnlockProcessor(
	IN OUT VOID *Buffer
)
{	
	UINT64 msr_program_buffer;
	UINT64 msr_ret;

	UINTN processor_index = DeviceExtension->NextPackage;

	// check MSR_FLEX_RATIO OC Lock Bit not set
	msr_ret = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	if ((msr_ret & MSR_FLEX_RATIO_OC_LOCK_BIT) != 0) {
		Print(
			L"Error: CPU%d MSR_FLEX_RATIO (0x194) OC Lock Bit [0x20] is set\r\n\0",
			processor_index
			);

		return;
	}

	#ifdef _DOMAIN0_VOLTAGE_ENABLED_

	// program Core voltage offset
	UINT64 msr_domain0_VID_program_buffer;
	
	msr_domain0_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_0 | OC_MAILBOX_COMMAND_EXEC) | \
		(kcpu_domain_0_voltage_offset[processor_index] | DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio);

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain0_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if ((msr_ret & OC_MAILBOX_RESPONSE_MASK) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox failure setting Core dynamic offset voltage\r\n\0",
			processor_index
			);
	}

	#endif // _DOMAIN0_VOLTAGE_ENABLED_

	#ifdef _DOMAIN2_VOLTAGE_ENABLED_

	// program CLR voltage offset
	UINT64 msr_domain2_VID_program_buffer;
	
	msr_domain2_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_2 | OC_MAILBOX_COMMAND_EXEC) | \
		(kcpu_domain_2_voltage_offset[processor_index] | DeviceExtension->ProcessorPackage[processor_index].MaxUncoreRatio);

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain2_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if ((msr_ret & OC_MAILBOX_RESPONSE_MASK) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox failure setting CLR (CBo/LLC/Ring) dynamic offset voltage\r\n\0",
			processor_index
			);
	}

	#endif // _DOMAIN2_VOLTAGE_ENABLED_

	#ifdef _DOMAIN3_VOLTAGE_ENABLED_
		
	// program SA voltage offset
	UINT64 msr_domain3_VID_program_buffer;
	
	msr_domain3_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_3 | OC_MAILBOX_COMMAND_EXEC) | \
		kcpu_domain_3_voltage_offset[processor_index];

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain3_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if ((msr_ret & OC_MAILBOX_RESPONSE_MASK) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox failure setting System Agent dynamic offset voltage\r\n\0",
			processor_index
			);
	}

	#endif // _DOMAIN3_VOLTAGE_ENABLED_
	
	#ifdef _DOMAIN0_CLOCKING_ENABLED_

	// set Core Turbo ratios
	UINT64 msr_domain0_clock_program_buffer;
	
	msr_domain0_clock_program_buffer = DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 8) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 16) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 24) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 32) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 40) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 48) | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio << 56);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT,
		msr_domain0_clock_program_buffer
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT1,
		msr_domain0_clock_program_buffer
		);

	msr_domain0_clock_program_buffer = msr_domain0_clock_program_buffer | \
		MSR_TURBO_RATIO_SEMAPHORE_BIT;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT2,
		msr_domain0_clock_program_buffer
		);

	// display results to console
	Print(
		L"CPU%d Success! %2d-core Turbo Frequency: %d MHz (= %d x %d MHz)\r\n\0",
		processor_index,
		DeviceExtension->SystemTopology->Cores[processor_index] / (DeviceExtension->SystemTopology->HyperThreading + 1),
		DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio * BUS_FREQUENCY,
		DeviceExtension->ProcessorPackage[processor_index].MaxTurboRatio,
		BUS_FREQUENCY
		);

	#endif // _DOMAIN0_CLOCKING_ENABLED_

	#ifdef _DOMAIN2_CLOCKING_ENABLED_

	// set Uncore ratios	
	UINT64 msr_domain2_clock_program_buffer;
	
	msr_ret = AsmReadMsr64(
		UNCORE_RATIO_LIMIT
		);

	msr_domain2_clock_program_buffer = (msr_ret & 0xFFFFFFFFFFFF0000ull) | \
		DeviceExtension->ProcessorPackage[processor_index].MaxUncoreRatio | \
		(DeviceExtension->ProcessorPackage[processor_index].MaxUncoreRatio << 8);

	AsmWriteMsr64(
		UNCORE_RATIO_LIMIT,
		msr_domain2_clock_program_buffer
		);

	Print(
		L"CPU%d Success! Static Uncore Frequency: %d MHz (= %d x %d MHz)\r\n\0",
		processor_index,
		DeviceExtension->ProcessorPackage[processor_index].MaxUncoreRatio * BUS_FREQUENCY,
		DeviceExtension->ProcessorPackage[processor_index].MaxUncoreRatio,
		BUS_FREQUENCY
		);

	#endif // _DOMAIN2_CLOCKING_ENABLED_

	#ifdef _SET_OVERCLOCKING_LOCK_
	
	// set MSR_FLEX_RATIO OC Lock Bit (no further changes without reboot)
	msr_ret = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	msr_program_buffer = msr_ret | \
		MSR_FLEX_RATIO_OC_LOCK_BIT;

	AsmWriteMsr64(
		MSR_FLEX_RATIO,
		msr_program_buffer
		);

	#endif // _SET_OVERCLOCKING_LOCK_
	
	return;
}

BOOLEAN
EFIAPI
IsMicrocodePresent(
	VOID
)
{
	UINT64 msr_ret;

	AsmWriteMsr64(
		MSR_IA32_BIOS_SIGN_ID,
		0
		);

	AsmCpuid(
		CPUID_VERSION_INFO,
		NULL,
		NULL,
		NULL,
		NULL
		);

	msr_ret = AsmReadMsr64(
		MSR_IA32_BIOS_SIGN_ID
		);

	if (((msr_ret & 0xFFFFFFFF00000000ull) >> 32) != 0) {
		return TRUE;
	}

	return FALSE;
}
