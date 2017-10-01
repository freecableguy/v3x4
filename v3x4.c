#include <PiPei.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Protocol/MpService.h>

// general definitions
#define		MAX_PACKAGE_COUNT		4			// maximum number of packages supported by the build
#define		BUS_FREQUENCY			100			// placeholder; TO DO: add bus frequency sensing
#define		CPUID_VERSION_INFO		0x00000001
#define		CPUID_BRAND_STRING_BASE		0x80000002
#define		CPUID_BRAND_STRING_LEN		48

// Model Specific Registers
#define		MSR_IA32_BIOS_SIGN_ID		0x0000008B
#define		MSR_PLATFORM_INFO		0x000000CE
#define		MSR_OC_MAILBOX			0x00000150		// no documentation available in the public domain
#define		MSR_FLEX_RATIO			0x00000194
#define		MSR_TURBO_RATIO_LIMIT		0x000001AD
#define		MSR_TURBO_RATIO_LIMIT1 		0x000001AE
#define		MSR_TURBO_RATIO_LIMIT2 		0x000001AF
#define		UNCORE_RATIO_LIMIT		0x00000620

// MSR status bits
#define		MSR_FLEX_RATIO_OC_LOCK_BIT	0x0000000000100000ull	// set to lock MSR 0x194 (RO until reboot)
#define		MSR_TURBO_RATIO_SEMAPHORE_BIT	0x8000000000000000ull	// set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1A 

// toolbox for OC Mailbox
#define		OC_MAILBOX_COMMAND_EXEC		0x8000000000000000ull
#define		OC_MAILBOX_GET_CPU_CAPS		0x0000000100000000ull
#define		OC_MAILBOX_SET_VID_PARAMS	0x0000001100000000ull
#define		OC_MAILBOX_DOMAIN_0		0x0000000000000000ull	// Core domain
#define		OC_MAILBOX_DOMAIN_2		0x0000020000000000ull	// CLR (CBo/LLC/Ring) domain
#define		OC_MAILBOX_DOMAIN_3		0x0000030000000000ull	// System Agent domain
#define		OC_MAILBOX_RESPONSE_MASK	0x000000FF00000000ull
#define		OC_MAILBOX_RESPONSE_SUCCESS	0

// negative dynamic voltage offsets for all domains
#define		_no_offset			0			// no change to factory voltage setting
#define		_neg_10_mV			0xFEC00000		// -10mV
#define		_neg_20_mV			0xFD800000		// -20mV
#define		_neg_30_mV			0xFC200000		// -30mV
#define		_neg_40_mV			0xFAE00000		// -40mV
#define		_neg_50_mV			0xF9A00000		// -50mV
#define		_neg_60_mV			0xF8600000		// -60mV
#define		_neg_70_mV			0xF7000000		// -70mV
#define		_neg_80_mV			0xF5C00000		// -80mV
#define		_neg_90_mV			0xF4800000		// -90mV
#define		_neg_100_mV			0xF3400000		// -100mV
#define		_neg_110_mV			0xF1E00000		// -110mV
#define		_neg_120_mV			0xF0A00000		// -120mV
#define		_neg_130_mV			0xEF600000		// -130mV
#define		_neg_140_mV			0xEE200000		// -140mV
#define		_neg_150_mV			0xECC00000		// -150mV

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

// ***EDIT HERE*** Build options ***EDIT HERE***
#define		_VERBOSE_OUTPUT_					// OPTION FLAG: more verbose console outputs during driver execute, comment out to disable
#define		_SET_OVERCLOCKING_LOCK_					// OPTION FLAG: set MSR 0x194[0x20] to prevent any changes following driver exit, comment out to disable
#define		_DOMAIN0_CLOCKING_ENABLED_				// FUNCTION FLAG: enable Core ratios to maximum turbo (all-core turbo) for maximum performance, comment out to disable
#define		_DOMAIN2_CLOCKING_ENABLED_				// FUNCTION FLAG: enable set static Uncore ratio to maximum performance, comment out to disble
#define		_DOMAIN0_VOLTAGE_ENABLED_				// SAFETY FLAG: enable programming Core voltages offsets, comment out to disable
#define		_DOMAIN2_VOLTAGE_ENABLED_				// SAFETY FLAG: enable programming CLR voltage offsets, comment out to disable
#define		_DOMAIN3_VOLTAGE_ENABLED_				// SAFETY FLAG: enable programming SA voltage offsets, comment out to disable

#define		_TARGET_CPU_CPUID_SIGN_		0x000306F2		// BUILD OPTION: set 0xFFFFFFFF to override and allow to attempt to run for all CPUs (with potentially unknown results)
																			// note: CPUID for Haswell-E/EP final QS/production silicon is 0x306F2 (Extended Family: 3F, Model: 6, Stepping: 2)

typedef struct _PLATFORM_OBJECT {
	UINTN				Packages;			// number of physical processor packages
	UINTN				LogicalProcessors;		// total number of logical processors
	UINTN				EnabledLogicalProcessors;	// total number of enabled logical processors
	UINTN				Cores[MAX_PACKAGE_COUNT];	// number cores in package[x]
	UINTN				Threads[MAX_PACKAGE_COUNT];	// number threads in package[x]
	UINTN				APICID[MAX_PACKAGE_COUNT];	// logical processor number for first core/thread in package[x]
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _PROCESSOR_PACKAGE_OBJECT {
	UINT32				ProcessorCPUID;			// processor CPUID
	CHAR16				BrandNameString[100];		// processor brand name Unicode string
	UINTN				MinFreqModeMulti;		// Minimum Frequency Mode (MFM) or "Low Power Mode" (LPM) multiplier
	UINTN				LowFreqModeMulti;		// Low Frequency Mode (LFM), minimum non-turbo Core multiplier
	UINTN				HighFreqModeMulti;		// High Frequecy Mode (HFM), maximum non-turbo Core multiplier
	UINTN				MaxTurboMulti;			// maximum (1C) Turbo Core multiplier
	UINTN				MaxUncoreMulti;			// maximum Uncore multiplier
} PROCESSOR_PACKAGE_OBJECT, *PPROCESSOR_PACKAGE_OBJECT;

typedef struct _V3DRIVER_DEVICE_EXTENTION {
	PPLATFORM_OBJECT		Platform;			// Platform object
	PPROCESSOR_PACKAGE_OBJECT	ProcessorPackage;		// array of Processor Package objects
	UINTN				BootstrapProcessor;		// bootstrap processor assignment at driver entry
	UINTN				NextPackage;			// package number of next package to be programmed/unlocked
} V3DRIVER_DEVICE_EXTENTION, *PV3DRIVER_DEVICE_EXTENTION;

// global variables
EFI_MP_SERVICES_PROTOCOL		*MpServicesProtocol;		// ptr to MP Services Protocol
V3DRIVER_DEVICE_EXTENTION		*DeviceExtension;		// ptr to device extension

// function prototypes
EFI_STATUS	EFIAPI InitializePlatform(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS	EFIAPI EnumProcessors(IN OUT PPROCESSOR_PACKAGE_OBJECT *ProcessorPackageObject);
VOID		EFIAPI ProgramPackage(IN OUT VOID *Buffer);
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
		InitializePlatform(
			&DeviceExtension->Platform
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

	for (UINTN package_index = 0; package_index < DeviceExtension->Platform->Packages; package_index++) {
		// program CPU using current AP if BSP already CPU0 (likely)
		if (DeviceExtension->BootstrapProcessor == DeviceExtension->Platform->APICID[DeviceExtension->NextPackage]) {
			ProgramPackage(NULL);			
		}
		else {
			// dispatch AP to program CPUx
			status = MpServicesProtocol->StartupThisAP(
				MpServicesProtocol,
				ProgramPackage,
				DeviceExtension->Platform->APICID[DeviceExtension->NextPackage],
				NULL,
				100000,  // 100 ms timeout
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
InitializePlatform(
	IN OUT PPLATFORM_OBJECT *PlatformObject
)
{
	// local variables
	EFI_PROCESSOR_INFORMATION processor_info;
	UINTN package_index;
	UINTN hyperthreading_detected = 0;
	UINTN thread_count = 0;
	EFI_STATUS status;
	
	// allocate memory pool space
	PPLATFORM_OBJECT Platform = (PPLATFORM_OBJECT)AllocatePool(
		sizeof(PLATFORM_OBJECT)
		);

	if (!Platform) {
		return EFI_OUT_OF_RESOURCES; 
	}

	// zero it
	SetMem(
		Platform,
		sizeof(PLATFORM_OBJECT),
		0
		);

	// set initial values
	Platform->Packages = 1;
		
	// get number of logical processors, enabled logical processors for entire system
	status = MpServicesProtocol->GetNumberOfProcessors(
		MpServicesProtocol,
		&Platform->LogicalProcessors,
		&Platform->EnabledLogicalProcessors
		);
	
	if (EFI_ERROR(status)) {
		return status;
	}
			
	// get number of threads per package, get APICID for first logical processor in each detected package,
	// derive number of cores based on HyperThreading detected enabled/disabled
	for (package_index = 0; package_index < Platform->LogicalProcessors; package_index++)
	{
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			package_index,
			&processor_info
			);

		// detect if HyperThreading enabled (platform-level feature)
		if (processor_info.Location.Thread == 1) {
			hyperthreading_detected = 1;
		}
				
		// count up the number of threads per package on a per-package basis
		thread_count++;

		// package ID changes -> new package found
		if (processor_info.Location.Package > (Platform->Packages - 1)) {
			Platform->Threads[processor_info.Location.Package - 1] = thread_count - 1;

			if (hyperthreading_detected == 0) {
				Platform->Cores[processor_info.Location.Package - 1] = Platform->Threads[processor_info.Location.Package - 1];
			}
			else {
				Platform->Cores[processor_info.Location.Package - 1] = Platform->Threads[processor_info.Location.Package - 1] / 2;
			}

			Platform->APICID[processor_info.Location.Package - 1] = package_index - thread_count + 1;

			// count up the number of packages
			Platform->Packages++;
			
			// rest thread count to start counting again
			thread_count = 0;
		}

		// last logical processor, no more new package detections
		if (package_index == (Platform->LogicalProcessors - 1)) {
			Platform->Threads[Platform->Packages - 1] = thread_count + 1;

			if (hyperthreading_detected == 0) {
				Platform->Cores[Platform->Packages - 1] = Platform->Threads[Platform->Packages - 1];
			}
			else {
				Platform->Cores[Platform->Packages - 1] = Platform->Threads[Platform->Packages - 1] / 2;
			}

			Platform->APICID[Platform->Packages - 1] = package_index - thread_count;
		}
	}
	
	#ifdef _VERBOSE_OUTPUT_

	Print(
		L"\r\n\0"
		);

	#endif // _VERBOSE_OUTPUT_
	
	// display results to console
	Print(L"Processor Packages: %d, Logical Processors: %d (%dC/%dT)\r\n\0",
		Platform->Packages,
		Platform->LogicalProcessors,
		Platform->LogicalProcessors / (hyperthreading_detected + 1),
		Platform->LogicalProcessors
		);

	#ifdef _VERBOSE_OUTPUT_
	
	UINTN package_count;

	for (package_count = 0; package_count < Platform->Packages; package_count++)
	{
		Print(L" -- Processor %d (CPU%d) Cores: %d, Threads: %d, APIC ID: %02xh\r\n\0",
			package_count,
			package_count,
			Platform->Cores[package_count],
			Platform->Threads[package_count], 
			Platform->APICID[package_count]
			);
	}

	#endif // _VERBOSE_OUTPUT_

	*PlatformObject = Platform;
	
	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EnumProcessors(
	IN OUT PPROCESSOR_PACKAGE_OBJECT *ProcessorPackageObject
)
{
	// local variables
	UINTN package_index;
	UINT64 msr_ret;
	
	// allocate memory pool space (one for each package detected)
	PPROCESSOR_PACKAGE_OBJECT ProcessorPackage = (PPROCESSOR_PACKAGE_OBJECT)AllocatePool(
		DeviceExtension->Platform->Packages * sizeof(PROCESSOR_PACKAGE_OBJECT)
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
	
	// initialize processor data
	for (package_index = 0; package_index < DeviceExtension->Platform->Packages; package_index++)
	{			
		// get processor CPUID
		AsmCpuid(
			CPUID_VERSION_INFO,
			&ProcessorPackage[package_index].ProcessorCPUID,
			NULL,
			NULL,
			NULL
			);

		// throw error if does not match target CPUID (unless override set)
		if ((ProcessorPackage[package_index].ProcessorCPUID != _TARGET_CPU_CPUID_SIGN_) \
			&& (_TARGET_CPU_CPUID_SIGN_ != 0xFFFFFFFF)) {
			Print(
				L"Error: CPU%d Processor CPUID (0x%x) not equal to 0x%x\r\n\0",
				package_index,
				ProcessorPackage[package_index].ProcessorCPUID,
				_TARGET_CPU_CPUID_SIGN_
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
					sizeof(UINT32)
					);

				k = k + sizeof(UINT32);
			}
		}
		
		processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

		// convert ASCII to Unicode
		AsciiStrToUnicodeStrS(
			processor_brand_string_buffer,
			ProcessorPackage[package_index].BrandNameString,
			sizeof(ProcessorPackage[package_index].BrandNameString)
			);

		// get Min/Low/High (LPM/LFM/HFM) non-turbo Core ratios
		msr_ret = AsmReadMsr64(
			MSR_PLATFORM_INFO
			);

		ProcessorPackage[package_index].MinFreqModeMulti = (msr_ret & 0x00FF000000000000ull) >> 48;
		
		ProcessorPackage[package_index].LowFreqModeMulti = (msr_ret & 0x0000FF0000000000ull) >> 40;
		
		ProcessorPackage[package_index].HighFreqModeMulti = (msr_ret & 0x000000000000FF00ull) >> 8;

		// get maximum Core Turbo ratio via OC Mailbox
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
				package_index
				);
			
			return EFI_ABORTED;
		}

		ProcessorPackage[package_index].MaxTurboMulti = msr_ret & 0x00000000000000FFull;

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
				package_index
				);
			
			return EFI_ABORTED;
		}

		ProcessorPackage[package_index].MaxUncoreMulti = msr_ret & 0x00000000000000FFull;
		
		#ifdef _VERBOSE_OUTPUT_

		Print(
			L"\r\n\0"
		);

		#endif // _VERBOSE_OUTPUT_
		
		// display results to console
		Print(
			L"Detected CPU%d: %s\r\n\0",
			package_index,
			ProcessorPackage[package_index].BrandNameString
			);

		#ifdef _VERBOSE_OUTPUT_

		Print(
			L" -- CPU%d Min Frequency Mode (MFM): %d MHz (= %d x %d MHz)\r\n\0",
			package_index,
			ProcessorPackage[package_index].MinFreqModeMulti * BUS_FREQUENCY,
			ProcessorPackage[package_index].MinFreqModeMulti,
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d Low Frequency Mode (LFM): %d MHz (= %d x %d MHz)\r\n\0", 
			package_index,
			ProcessorPackage[package_index].LowFreqModeMulti * BUS_FREQUENCY,
			ProcessorPackage[package_index].LowFreqModeMulti,
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d High Frequency Mode (HFM): %d MHz (= %d x %d MHz)\r\n\0",
			package_index,
			ProcessorPackage[package_index].HighFreqModeMulti * BUS_FREQUENCY,
			ProcessorPackage[package_index].HighFreqModeMulti,
			BUS_FREQUENCY
			);

		Print(L" -- CPU%d Max Core Turbo Frequency: %d MHz (= %d x %d MHz)\r\n\0",
			package_index,
			ProcessorPackage[package_index].MaxTurboMulti * BUS_FREQUENCY,
			ProcessorPackage[package_index].MaxTurboMulti,
			BUS_FREQUENCY
			);

		Print(
			L" -- CPU%d Max Uncore Frequency: %d MHz (= %d x %d MHz)\r\n\0",
			package_index,
			ProcessorPackage[package_index].MaxUncoreMulti * BUS_FREQUENCY,
			ProcessorPackage[package_index].MaxUncoreMulti,
			BUS_FREQUENCY
			);
		
		#endif // _VERBOSE_OUTPUT_		
	}
	
	#ifdef _VERBOSE_OUTPUT_

	Print(
		L"\r\n\0"
	);

	#endif // _VERBOSE_OUTPUT_

	*ProcessorPackageObject = ProcessorPackage;
	
	return EFI_SUCCESS;
}

// must be run the context of the package to be programmed
VOID
EFIAPI
ProgramPackage(
	IN OUT VOID *Buffer
)
{	
	// local variables
	UINT64 msr_program_buffer;
	UINT64 msr_ret;

	// target package to be programmed
	UINTN processor_index = DeviceExtension->NextPackage;

	// verify MSR_FLEX_RATIO OC Lock Bit not set
	msr_ret = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	if ((msr_ret & MSR_FLEX_RATIO_OC_LOCK_BIT) != 0) {
		Print(
			L"Error: CPU%d MSR_FLEX_RATIO (0x194) OC Lock Bit (0x20) is set\r\n\0",
			processor_index
			);

		return;
	}

	#ifdef _DOMAIN0_VOLTAGE_ENABLED_

	// program Core voltage offset
	UINT64 msr_domain0_VID_program_buffer;
	
	msr_domain0_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_0 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_0_voltage_offset[processor_index] \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti;

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
	
	msr_domain2_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_2 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_2_voltage_offset[processor_index] \
		| DeviceExtension->ProcessorPackage[processor_index].MaxUncoreMulti;

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
	
	msr_domain3_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_3 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_3_voltage_offset[processor_index];

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
	
	msr_domain0_clock_program_buffer = DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 8  \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 16 \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 24 \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 32 \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 40 \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 48 \
		| DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti << 56;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT,
		msr_domain0_clock_program_buffer
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT1,
		msr_domain0_clock_program_buffer
		);

	msr_domain0_clock_program_buffer = msr_domain0_clock_program_buffer \
		| MSR_TURBO_RATIO_SEMAPHORE_BIT;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT2,
		msr_domain0_clock_program_buffer
		);

	// display results to console
	Print(
		L"CPU%d Success! %2d-core Turbo Frequency: %d MHz (= %d x %d MHz)\r\n\0",
		processor_index,
		DeviceExtension->Platform->Cores[processor_index],
		DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti * BUS_FREQUENCY,
		DeviceExtension->ProcessorPackage[processor_index].MaxTurboMulti,
		BUS_FREQUENCY
		);

	#endif // _DOMAIN0_CLOCKING_ENABLED_

	#ifdef _DOMAIN2_CLOCKING_ENABLED_

	// set Uncore ratios	
	UINT64 msr_domain2_clock_program_buffer;
	
	msr_ret = AsmReadMsr64(
		UNCORE_RATIO_LIMIT
		);

	msr_domain2_clock_program_buffer = (msr_ret & 0xFFFFFFFFFFFF0000ull) \
		| DeviceExtension->ProcessorPackage[processor_index].MaxUncoreMulti \
		| (DeviceExtension->ProcessorPackage[processor_index].MaxUncoreMulti << 8);

	AsmWriteMsr64(
		UNCORE_RATIO_LIMIT,
		msr_domain2_clock_program_buffer
		);

	Print(
		L"CPU%d Success! Static Uncore Frequency: %d MHz (= %d x %d MHz)\r\n\0",
		processor_index,
		DeviceExtension->ProcessorPackage[processor_index].MaxUncoreMulti * BUS_FREQUENCY,
		DeviceExtension->ProcessorPackage[processor_index].MaxUncoreMulti,
		BUS_FREQUENCY
		);

	#endif // _DOMAIN2_CLOCKING_ENABLED_

	#ifdef _SET_OVERCLOCKING_LOCK_
	
	// set MSR_FLEX_RATIO OC Lock Bit
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
	// local variables
	UINT64 msr_ret;

	// get microcode revision level
	AsmWriteMsr64(
		MSR_IA32_BIOS_SIGN_ID,
		0x00000000ul
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

	// high word contains microcode update revision patch level (should be 0 for no patch loaded)
	if (((msr_ret & 0xFFFFFFFF00000000ull) >> 32) != 0) {
		return TRUE;
	} 
	else {
		return FALSE;
	}
}
