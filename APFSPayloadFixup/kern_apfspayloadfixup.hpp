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
	/**
	 *  Original method
	 */
	mach_vm_address_t org_vfs_mountroot {};

	
	int64_t *gARV_payload_bytes {nullptr};
	int64_t *gARV_payload_len   {nullptr};
};

#endif /* kern_trimdisabler_hpp */
