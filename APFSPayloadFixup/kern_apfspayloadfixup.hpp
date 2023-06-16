//
//  kern_apfspayloadfixup.hpp
//  APFSPayloadFixup
//
//  Copyright © 2023 lvs1974. All rights reserved.
//

#ifndef kern_trimdisabler_hpp
#define kern_trimdisabler_hpp

#include <Headers/kern_patcher.hpp>
#include <stdatomic.h>

#include <IOKit/IOService.h>

class APFSPayloadFx {
public:
	bool init();
	void deinit();
	
private:
	/**
	 *  Patch kernel
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);
	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Hooked methods / callbacks
	 */
	static int vfs_mountroot();
    static bool AppleAPFSContainer_start(IOService* service, IOService* provider);
	static int64_t external_module_resources();
	/**
	 *  Original method
	 */
	mach_vm_address_t org_vfs_mountroot {};
	mach_vm_address_t org_external_module_resources {};
    mach_vm_address_t orgAppleAPFSContainer_start {};

	
	int64_t *gARV_payload_bytes {nullptr};
	int64_t *gARV_payload_len   {nullptr};
    
    int64_t last_used_payload_bytes = 0;
    int64_t last_payload_len = 0;
};

#endif /* kern_trimdisabler_hpp */
