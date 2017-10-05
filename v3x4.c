#include <PiPei.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Protocol/MpService.h>

// negative dynamic voltage offsets for all domains
#define		_no_offset				0			// no change to factory voltage setting
#define		_neg_10_mV				0xFEC00000		//  -10 mV (-0.010 V)
#define		_neg_20_mV				0xFD800000		//  -20 mV (-0.020 V)
#define		_neg_30_mV				0xFC200000		//  -30 mV (-0.030 V)
#define		_neg_40_mV				0xFAE00000		//  -40 mV (-0.040 V)
#define		_neg_50_mV				0xF9A00000		//  -50 mV (-0.050 V)
#define		_neg_60_mV				0xF8600000		//  -60 mV (-0.060 V)
#define		_neg_70_mV				0xF7000000		//  -70 mV (-0.070 V)
#define		_neg_80_mV				0xF5C00000		//  -80 mV (-0.080 V)
#define		_neg_90_mV				0xF4800000		//  -90 mV (-0.090 V)
#define		_neg_100_mV				0xF3400000		// -100 mV (-0.100 V)
#define		_neg_110_mV				0xF1E00000		// -110 mV (-0.110 V)
#define		_neg_120_mV				0xF0A00000		// -120 mV (-0.120 V)
#define		_neg_130_mV				0xEF600000		// -130 mV (-0.130 V)
#define		_neg_140_mV				0xEE200000		// -140 mV (-0.140 V)
#define		_neg_150_mV				0xECC00000		// -150 mV (-0.150 V)

// Model Specific Registers
#define		MSR_IA32_BIOS_SIGN_ID			0x08B
#define		MSR_PLATFORM_INFO			0x0CE
#define		MSR_OC_MAILBOX				0x150
#define		MSR_FLEX_RATIO				0x194
#define		MSR_TURBO_RATIO_LIMIT			0x1AD
#define		MSR_TURBO_RATIO_LIMIT1 			0x1AE
#define		MSR_TURBO_RATIO_LIMIT2 			0x1AF
#define		MSR_UNCORE_RATIO_LIMIT			0x620

#define		MSR_FLEX_RATIO_OC_LOCK_BIT		0x0000000000100000ull	// set to lock MSR 0x194[0x20]
#define		MSR_TURBO_RATIO_SEMAPHORE_BIT		0x8000000000000000ull	// set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF

// toolbox for MSR OC Mailbox (experimental)
#define		OC_MAILBOX_COMMAND_EXEC			0x8000000000000000ull

#define		OC_MAILBOX_GET_CPU_CAPS			0x0000000100000000ull

#define		OC_MAILBOX_SET_VID_PARAMS		0x0000001100000000ull

#define		OC_MAILBOX_SET_FIVR_PARAMS		0x0000001500000000ull
#define		OC_MAILBOX_IGNORE_FIVR_FAULTS		1			// bit 0
#define		OC_MAILBOX_DISABLE_FIVR_EFFICIENCY	2			// bit 1

#define		OC_MAILBOX_DOMAIN_0			0x0000000000000000ull	// IA (Core) domain
#define		OC_MAILBOX_DOMAIN_2			0x0000020000000000ull	// CLR (CBo/LLC/Ring) a.k.a. Uncore domain
#define		OC_MAILBOX_DOMAIN_3			0x0000030000000000ull	// System Agent (SA) domain

#define		OC_MAILBOX_RESPONSE_MASK		0x000000FF00000000ull
#define		OC_MAILBOX_RESPONSE_SUCCESS		0

// constants
#define		BUS_FREQUENCY				100			// TO DO: placeholder for BCLK sense
#define		AP_EXEC_TIMEOUT				2000000			// 2 seconds
#define		CPUID_VERSION_INFO			1
#define		CPUID_BRAND_STRING_BASE			0x80000002
#define		CPUID_BRAND_STRING_LEN			48
#define		MIN_UNCORE_MULTI			12

// builds options
#define		TARGET_CPU_CPUID_SIGN			0x000306F2		// target CPUID, set 0xFFFFFFFF to allow to continue regardless of CPUID
#define		LIMIT_TURBO_MULTI			0			// 0 for auto max turbo multiplier, not to exceed fused limit, no less than MFM (12)
#define		LIMIT_UNCORE_MUTLI			0			// 0 for auto max Uncore multiplier, not to exceed fused limit, no less than 12
#define		VERBOSE_OUTPUT				TRUE			// more verbose driver information during system boot
#define		SET_STATIC_UNCORE_FREQ			TRUE			// TRUE = set static Uncore frequency (at LIMIT_UNCORE_MULTI)
#define		DISABLE_FIVR_FAULT_CONTROL		TRUE			// TRUE = disable FIVR fault detection
#define		DISABLE_FIVR_EFFICIENCY_MODE		TRUE			// TRUE = disable FIVR efficiency mode
#define		SET_OVERCLOCK_LOCK			TRUE			// TRUE = set MSR 0x194[0x20] to prevent any later changes to this MSR, cleared on reset
#define		MAX_PACKAGE_COUNT			2			// maximum number of supported packages/sockets, options: 1, 2, 4, 8

// Domain 0 (IA) dynamic voltage offsets per package
const UINT32 kcpu_domain_0_voltage_offset[MAX_PACKAGE_COUNT] \
= { _neg_50_mV, _neg_50_mV }; // , _no_offset, _no_offset, _no_offset, _no_offset, _no_offset, _no_offset };

// Domain 2 (CLR) dynamic voltage offsets per package
const UINT32 kcpu_domain_2_voltage_offset[MAX_PACKAGE_COUNT] \
= { _neg_50_mV, _neg_50_mV }; // , _no_offset, _no_offset, _no_offset, _no_offset, _no_offset, _no_offset };

// Domain 3 (SA) dynamic voltage offsets per package
const UINT32 kcpu_domain_3_voltage_offset[MAX_PACKAGE_COUNT] \
= { _neg_50_mV, _neg_50_mV }; // , _no_offset, _no_offset, _no_offset, _no_offset, _no_offset, _no_offset };

// object structures
typedef struct _PLATFORM_OBJECT {
	UINTN			Packages;					// number of physical processor packages
	UINTN			LogicalProcessors;				// total number of logical processors
	UINTN			EnabledLogicalProcessors;			// total number of enabled logical processors (same as logical processors unless AP disabled)
	UINTN			Cores[MAX_PACKAGE_COUNT];			// number cores in package
	UINTN			Threads[MAX_PACKAGE_COUNT];			// number threads in package
	UINTN			APICID[MAX_PACKAGE_COUNT];			// logical processor number for first core/thread in package (i.e. APIC ID)
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _PACKAGE_OBJECT {
	UINT32			CPUID;						// CPUID
	CHAR16			Specification[100];				// processor brand name
	UINTN			MFMMulti;					// Minimum Frequency Mode (MFM) or Low Power Mode (LPM) multiplier
	UINTN			LFMMulti;					// Low Frequency Mode (LFM) multiplier, min non-turbo Core multiplier
	UINTN			HFMMulti;					// High Frequecy Mode (HFM) multiplier, max non-turbo Core multiplier
	UINTN			Turbo1CMulti;					// max (1C) Turbo multiplier
	UINTN			TurboMultiLimit;				// highest allowed turbo mulitplier
	UINTN			MaxUncoreMulti;					// max possible Uncore multiplier
	UINTN			UncoreMultiLimit;				// highest allowed Uncore multiplier
} PACKAGE_OBJECT, *PPACKAGE_OBJECT;

typedef struct _SYSTEM_OBJECT {
	PPLATFORM_OBJECT	Platform;					// Platform object
	PPACKAGE_OBJECT		Package;					// array of Package objects dynamically built during execution
	UINTN			BootstrapProcessor;				// bootstrap processor assignment at driver entry
	UINTN			NextPackage;					// package number of next Package to be programmed
} SYSTEM_OBJECT, *PSYSTEM_OBJECT;

// global variables
EFI_MP_SERVICES_PROTOCOL	*MpServicesProtocol;				// ptr to MP Services Protocol handle
SYSTEM_OBJECT			*System;					// ptr to base System object

// function prototypes
EFI_STATUS	EFIAPI InitPlatform(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS	EFIAPI EnumProcessors(IN OUT PPACKAGE_OBJECT *PackageObject);
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
	// driver init
	EFI_STATUS status = SystemTable->ConOut->OutputString(
		SystemTable->ConOut,
		L"Intel(R) Xeon(R) Processor v3 (Haswell-E/EP) Full Turbo Boost DXE driver\r\n\0"
		);

	if (EFI_ERROR(status)) {
		goto DriverExit;
	}

	// get handle to UEFI MP Services Protocol
	EFI_GUID efi_mp_service_protocol_guid = EFI_MP_SERVICES_PROTOCOL_GUID;
	
	status = SystemTable->BootServices->LocateProtocol(
		&efi_mp_service_protocol_guid,
		NULL,
		(VOID**)&MpServicesProtocol
		);

	if (EFI_ERROR(status)) {
		Print(
			L"Failure: Unable to locate EFI MP Services Protocol (%r)\r\n\0",
			status
			);
		
		goto DriverExit; 
	}

	// check no CPU microcode revision update patch is loaded
	if (IsMicrocodePresent() == TRUE) {
		Print(
			L"Failure: Processor microcode update revision detected\r\n\0"
			);

		goto DriverExit;
	}

	// create System object
	System = (PSYSTEM_OBJECT)AllocatePool(
		sizeof(SYSTEM_OBJECT)
		);

	if (!System) {
		goto DriverExit;
	}
	else {
		// zero it
		SetMem(
			System,
			sizeof(SYSTEM_OBJECT),
			0
			);
	}
		
	// get default BSP
	status = MpServicesProtocol->WhoAmI(
		MpServicesProtocol,
		&System->BootstrapProcessor
		);

	if (EFI_ERROR(status)) {
		Print(
			L"Failure: Cannot get current bootstrap processor (%r)\r\n\0",
			status
			);

		goto DriverExit;
	}

	// initialize Platform
	if (EFI_ERROR(
		InitPlatform(
			&System->Platform))) {
		goto DriverExit;
	}

	// enumerate Packages
	if (EFI_ERROR(
		EnumProcessors(
			&System->Package))) {
		goto DriverExit;
	}

	for (System->NextPackage ; System->NextPackage < System->Platform->Packages; System->NextPackage++) {
		// program CPU using current AP if same as current BSP
		if (System->BootstrapProcessor == System->Platform->APICID[System->NextPackage]) {
			ProgramPackage(
				NULL
				);			
		}
		else {
			// dispatch AP to program CPU
			status = MpServicesProtocol->StartupThisAP(
				MpServicesProtocol,
				ProgramPackage,
				System->Platform->APICID[System->NextPackage],
				NULL,
				AP_EXEC_TIMEOUT,
				NULL,
				NULL
				);

			if (EFI_ERROR(status)) {
				Print(
					L"Warning: Failed to startup programming AP on CPU%d (%r)\r\n\0",
					System->NextPackage,
					status
					);
			}
		}
	}
	
DriverExit:

	// always return success
	return EFI_SUCCESS;
}

// creates Platform object and intializes all member variables
EFI_STATUS
EFIAPI
InitPlatform(
	IN OUT PPLATFORM_OBJECT *PlatformObject
)
{
	// allocate memory
	PPLATFORM_OBJECT Platform = (PPLATFORM_OBJECT)AllocatePool(
		sizeof(PLATFORM_OBJECT)
		);

	if (!Platform) {
		return EFI_OUT_OF_RESOURCES; 
	}
	else {
		// zero it
		SetMem(
			Platform,
			sizeof(PLATFORM_OBJECT),
			0
			);
	}
	
	// set initial value
	Platform->Packages = 1;
	
	// get number of logical processors, enabled logical processors for entire system
	EFI_STATUS status = MpServicesProtocol->GetNumberOfProcessors(
		MpServicesProtocol,
		&Platform->LogicalProcessors,
		&Platform->EnabledLogicalProcessors
		);
	
	if (EFI_ERROR(status)) {
		return status;
	}

	EFI_PROCESSOR_INFORMATION processor_info;
	UINTN thread_counter = 0;
	UINTN htt_enabled = 0;

	for (UINTN thread_index = 0; thread_index < Platform->LogicalProcessors; thread_index++) {
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			thread_index,
			&processor_info
			);

		// detect if HyperThreading enabled
		if (processor_info.Location.Thread == 1) {
			htt_enabled = 1;
		}
		
		thread_counter++;

		// last logical processor
		if (thread_index == (Platform->LogicalProcessors - 1)) {
			Platform->Threads[System->NextPackage] = thread_counter;

			Platform->Cores[System->NextPackage] = Platform->Threads[System->NextPackage] / (htt_enabled + 1);

			Platform->APICID[System->NextPackage] = (thread_index - thread_counter) + 1;

			break;
		}
	
		// package ID changes -> new package found
		if (processor_info.Location.Package != System->NextPackage) {
			Platform->Packages++;
			
			Platform->Threads[System->NextPackage] = thread_counter - 1;

			Platform->Cores[System->NextPackage] = Platform->Threads[System->NextPackage] / (htt_enabled + 1);
			
			Platform->APICID[System->NextPackage] = (thread_index - thread_counter) + 1;
			
			System->NextPackage++;

			thread_counter = 1;
		}
	}

	System->NextPackage = 0;
	
	if (VERBOSE_OUTPUT == TRUE) {
		Print(
			L"\r\n\0"
			);
	}
	
	Print(L"Processor Packages: %d, Logical Processors: %d (%dC/%dT)\r\n\0",
		Platform->Packages,
		Platform->LogicalProcessors,
		Platform->LogicalProcessors / (htt_enabled + 1),
		Platform->LogicalProcessors
		);

	if (VERBOSE_OUTPUT == TRUE) {
		for (System->NextPackage; System->NextPackage < Platform->Packages; System->NextPackage++) {
			Print(L" -- Processor %d (CPU%d) Cores: %d, Threads: %d, APIC ID: %02xh\r\n\0",
				System->NextPackage,
				System->NextPackage,
				Platform->Cores[System->NextPackage],
				Platform->Threads[System->NextPackage],
				Platform->APICID[System->NextPackage]
				);
		}
		
		System->NextPackage = 0;
	}

	*PlatformObject = Platform;
	
	return EFI_SUCCESS;
}

// creates array of Package objects and intializes all member variables
EFI_STATUS
EFIAPI
EnumProcessors(
	IN OUT PPACKAGE_OBJECT *PackageObject
)
{
	// allocate memory
	PPACKAGE_OBJECT Package = (PPACKAGE_OBJECT)AllocatePool(
		System->Platform->Packages * sizeof(PACKAGE_OBJECT)
		);
	
	if (!Package) {
		return EFI_OUT_OF_RESOURCES;
	}
	else {
		// zero it
		SetMem(
			Package, 
			sizeof(PACKAGE_OBJECT),
			0
			);
	}
		
	// initialize processor data
	for (System->NextPackage; System->NextPackage < System->Platform->Packages; System->NextPackage++) {			
		// get processor CPUID
		AsmCpuid(
			CPUID_VERSION_INFO,
			&Package[System->NextPackage].CPUID,
			NULL,
			NULL,
			NULL
			);

		// build processor brand name string		
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

				k += 4;
			}
		}
		
		processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

		// convert ASCII to Unicode
		AsciiStrToUnicodeStrS(
			processor_brand_string_buffer,
			Package[System->NextPackage].Specification,
			sizeof(Package[System->NextPackage].Specification)
			);

		// get MPM(LPM)/LFM/HFM non-turbo multipliers
		UINT64 msr_ret = AsmReadMsr64(
			MSR_PLATFORM_INFO
			);

		Package[System->NextPackage].MFMMulti = (msr_ret & 0x00FF000000000000ull) >> 48;
		
		Package[System->NextPackage].LFMMulti = (msr_ret & 0x0000FF0000000000ull) >> 40;
		
		Package[System->NextPackage].HFMMulti = (msr_ret & 0x000000000000FF00ull) >> 8;

		// get maximum (1C) turbo multiplier
		AsmWriteMsr64(
			MSR_OC_MAILBOX,
			OC_MAILBOX_GET_CPU_CAPS | OC_MAILBOX_DOMAIN_0 | OC_MAILBOX_COMMAND_EXEC
			);

		msr_ret = AsmReadMsr64(
			MSR_OC_MAILBOX
			);

		if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
			Print(
				L"Failure: CPU%d OC Mailbox error reading max turbo multiplier\r\n\0",
				System->NextPackage
				);
			
			return EFI_ABORTED;
		}

		Package[System->NextPackage].Turbo1CMulti = msr_ret & 0x00000000000000FFull;

		// set turbo multi limit
		if (LIMIT_TURBO_MULTI > Package[System->NextPackage].Turbo1CMulti) {
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].Turbo1CMulti;
		}

		if ((LIMIT_TURBO_MULTI < Package[System->NextPackage].LFMMulti) \
			&& (LIMIT_TURBO_MULTI != 0)) {
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].LFMMulti;
		}

		if (LIMIT_TURBO_MULTI == 0) { // auto max
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].Turbo1CMulti;
		}
		else {
			Package[System->NextPackage].TurboMultiLimit = LIMIT_TURBO_MULTI;
		}

		// get maximum Uncore multiplier
		AsmWriteMsr64(
			MSR_OC_MAILBOX,
			OC_MAILBOX_GET_CPU_CAPS | OC_MAILBOX_DOMAIN_2 | OC_MAILBOX_COMMAND_EXEC
			);

		msr_ret = AsmReadMsr64(
			MSR_OC_MAILBOX
			);

		if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
			Print(
				L"Failure: CPU%d OC Mailbox error reading max SA/Uncore multiplier\r\n\0",
				System->NextPackage
				);
			
			return EFI_ABORTED;
		}

		Package[System->NextPackage].MaxUncoreMulti = msr_ret & 0x00000000000000FFull;
		
		// set Uncore multi limit
		if (LIMIT_UNCORE_MUTLI > Package[System->NextPackage].MaxUncoreMulti) {
			Package[System->NextPackage].UncoreMultiLimit = Package[System->NextPackage].MaxUncoreMulti;
		}
	
		if ((LIMIT_UNCORE_MUTLI < MIN_UNCORE_MULTI) \
			&& (LIMIT_UNCORE_MUTLI != 0)) {
			Package[System->NextPackage].UncoreMultiLimit = MIN_UNCORE_MULTI;
		}
	
		if (LIMIT_UNCORE_MUTLI == 0) { // auto max
			Package[System->NextPackage].UncoreMultiLimit = Package[System->NextPackage].MaxUncoreMulti;
		}
		else {
			Package[System->NextPackage].UncoreMultiLimit = LIMIT_UNCORE_MUTLI;
		}

		if (VERBOSE_OUTPUT == TRUE) {
			Print(
				L"\r\n\0"
				);
		}
		
		Print(
			L"Detected CPU%d: %s\r\n\0",
			System->NextPackage,
			Package[System->NextPackage].Specification
			);

		if (VERBOSE_OUTPUT == TRUE) {
			Print(
				L" -- Min Frequency Mode (MFM/LPM): %d MHz (= %d x %d MHz)\r\n\0",
				Package[System->NextPackage].MFMMulti * BUS_FREQUENCY,
				Package[System->NextPackage].MFMMulti,
				BUS_FREQUENCY
				);

			Print(
				L" -- Low Frequency Mode (LFM): %d MHz (= %d x %d MHz)\r\n\0",
				Package[System->NextPackage].LFMMulti * BUS_FREQUENCY,
				Package[System->NextPackage].LFMMulti,
				BUS_FREQUENCY
				);

			Print(
				L" -- High Frequency Mode (HFM): %d MHz (= %d x %d MHz)\r\n\0",
				Package[System->NextPackage].HFMMulti * BUS_FREQUENCY,
				Package[System->NextPackage].HFMMulti,
				BUS_FREQUENCY
				);

			Print(L" -- Max (1C) turbo frequency: %d MHz (= %d x %d MHz)\r\n\0",
				Package[System->NextPackage].Turbo1CMulti * BUS_FREQUENCY,
				Package[System->NextPackage].Turbo1CMulti,
				BUS_FREQUENCY
				);

			Print(
				L" -- Max Uncore frequency: %d MHz (= %d x %d MHz)\r\n\0",
				Package[System->NextPackage].MaxUncoreMulti * BUS_FREQUENCY,
				Package[System->NextPackage].MaxUncoreMulti,
				BUS_FREQUENCY
				);
		}
	}

	if (VERBOSE_OUTPUT == TRUE) {
		Print(
			L"\r\n\0"
			);
	}

	System->NextPackage = 0;

	*PackageObject = Package;
	
	return EFI_SUCCESS;
}

// programs package; MUST be executed in context of package to be programmed
VOID
EFIAPI
ProgramPackage(
	IN OUT VOID *Buffer
)
{	
	// verify package CPUID matchs build target CPUID
	if ((System->Package[System->NextPackage].CPUID != TARGET_CPU_CPUID_SIGN) \
		&& (TARGET_CPU_CPUID_SIGN != 0xFFFFFFFF)) {  // override
		Print(
			L"Failure: CPU%d CPUID (0x%x) not equal to 0x%x\r\n\0",
			System->NextPackage,
			System->Package[System->NextPackage].CPUID,
			TARGET_CPU_CPUID_SIGN
			);

		return;
	}
	
	// check OC Lock Bit not already set
	UINT64 msr_ret = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	if ((msr_ret & MSR_FLEX_RATIO_OC_LOCK_BIT) != 0) {
		Print(
			L"Failure: CPU%d MSR_FLEX_RATIO (0x194) OC Lock Bit (20h) is set\r\n\0",
			System->NextPackage
			);

		return;
	}

	// set turbo frequency limits
	UINT64 msr_domain0_clock_program_buffer = System->Package[System->NextPackage].TurboMultiLimit \
		| System->Package[System->NextPackage].TurboMultiLimit << 8  \
		| System->Package[System->NextPackage].TurboMultiLimit << 16 \
		| System->Package[System->NextPackage].TurboMultiLimit << 24 \
		| System->Package[System->NextPackage].TurboMultiLimit << 32 \
		| System->Package[System->NextPackage].TurboMultiLimit << 40 \
		| System->Package[System->NextPackage].TurboMultiLimit << 48 \
		| System->Package[System->NextPackage].TurboMultiLimit << 56;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT,
		msr_domain0_clock_program_buffer
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT1,
		msr_domain0_clock_program_buffer
		);

	msr_domain0_clock_program_buffer |= MSR_TURBO_RATIO_SEMAPHORE_BIT;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT2,
		msr_domain0_clock_program_buffer
		);

	Print(
		L"Success! Set CPU%d %2dC turbo frequency limit: %d MHz (= %d x %d MHz)\r\n\0",
		System->NextPackage,
		System->Platform->Cores[System->NextPackage],
		System->Package[System->NextPackage].TurboMultiLimit * BUS_FREQUENCY,
		System->Package[System->NextPackage].TurboMultiLimit,
		BUS_FREQUENCY
		);

	// set Uncore frequency limits	
	msr_ret = AsmReadMsr64(
		MSR_UNCORE_RATIO_LIMIT
		);

	UINT64 msr_domain2_clock_program_buffer = (msr_ret & 0xFFFFFFFFFFFFFF00ull) \
		| System->Package[System->NextPackage].UncoreMultiLimit;

	if (SET_STATIC_UNCORE_FREQ == TRUE) {
		msr_domain2_clock_program_buffer &= 0xFFFFFFFFFFFF00FFull;
		msr_domain2_clock_program_buffer |= (System->Package[System->NextPackage].UncoreMultiLimit << 8);
	}

	AsmWriteMsr64(
		MSR_UNCORE_RATIO_LIMIT,
		msr_domain2_clock_program_buffer
		);

	Print(
		L"Success! Set CPU%d Uncore frequency limit: %d MHz (= %d x %d MHz)\r\n\0",
		System->NextPackage,
		System->Package[System->NextPackage].UncoreMultiLimit * BUS_FREQUENCY,
		System->Package[System->NextPackage].UncoreMultiLimit,
		BUS_FREQUENCY
		);

	// set IA dynamic voltage offset
	UINT64 msr_domain0_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_0 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_0_voltage_offset[System->NextPackage] \
		| System->Package[System->NextPackage].Turbo1CMulti;

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain0_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox error setting Core dynamic offset voltage\r\n\0",
			System->NextPackage
			);
	}

	// set CLR dynamic voltage offset
	UINT64 msr_domain2_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_2 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_2_voltage_offset[System->NextPackage] \
		| System->Package[System->NextPackage].MaxUncoreMulti;

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain2_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox error setting CLR dynamic offset voltage\r\n\0",
			System->NextPackage
			);
	}

	// set SA/Uncore dynamic voltage offset
	UINT64 msr_domain3_VID_program_buffer = (OC_MAILBOX_SET_VID_PARAMS | OC_MAILBOX_DOMAIN_3 | OC_MAILBOX_COMMAND_EXEC) \
		| kcpu_domain_3_voltage_offset[System->NextPackage];

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_domain3_VID_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox error setting SA/Uncore dynamic offset voltage\r\n\0",
			System->NextPackage
			);
	}

	// set FIVR control parameters
	UINT64 msr_FIVR_program_buffer = OC_MAILBOX_SET_FIVR_PARAMS | OC_MAILBOX_COMMAND_EXEC;
		
	if (DISABLE_FIVR_FAULT_CONTROL == TRUE) {
		msr_FIVR_program_buffer |= OC_MAILBOX_IGNORE_FIVR_FAULTS;
	}

	if (DISABLE_FIVR_EFFICIENCY_MODE == TRUE) {
		msr_FIVR_program_buffer |= OC_MAILBOX_DISABLE_FIVR_EFFICIENCY;
	}

	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		msr_FIVR_program_buffer
		);

	msr_ret = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	if (((msr_ret & OC_MAILBOX_RESPONSE_MASK) >> 32) != OC_MAILBOX_RESPONSE_SUCCESS) {
		Print(
			L"Warning: CPU%d OC Mailbox error setting FIVR control parameters\r\n\0",
			System->NextPackage
			);
	}
		
	// optionally set OC Lock
	if (SET_OVERCLOCK_LOCK == TRUE) {
		UINT64 msr_program_buffer;
	
		msr_ret = AsmReadMsr64(
			MSR_FLEX_RATIO
			);

		msr_program_buffer = msr_ret | MSR_FLEX_RATIO_OC_LOCK_BIT;

		AsmWriteMsr64(
			MSR_FLEX_RATIO,
			msr_program_buffer
			);
	}

	return;
}

BOOLEAN
EFIAPI
IsMicrocodePresent(
	VOID
)
{
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

	UINT64 msr_ret = AsmReadMsr64(
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
