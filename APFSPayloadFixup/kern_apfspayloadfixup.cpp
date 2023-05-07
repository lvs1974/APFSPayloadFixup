//
//  kern_apfspayloadfixup.cpp
//  APFSPayloadFixup
//
//  Copyright © 2023 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include "kern_apfspayloadfixup.hpp"

static APFSPayloadFx *callbackAPFSPayloadFx = nullptr;

static const char *kextPathAPFS[] { "/System/Library/Extensions/apfs.kext/Contents/MacOS/apfs" };

static KernelPatcher::KextInfo kextList { "com.apple.filesystems.apfs", kextPathAPFS, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded };

bool APFSPayloadFx::init()
{
	callbackAPFSPayloadFx = this;

	lilu.onPatcherLoadForce(
	[](void *user, KernelPatcher &patcher) {
		callbackAPFSPayloadFx->processKernel(patcher);
	}, this);
	
	lilu.onKextLoadForce(&kextList, 1,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		callbackAPFSPayloadFx->processKext(patcher, index, address, size);
	}, this);

	return true;
}

//==============================================================================

void APFSPayloadFx::deinit()
{
}

//==============================================================================

int APFSPayloadFx::vfs_mountroot()
{
	DBGLOG("APFSPAYLOADFX", "vfs_mountroot is called");
	while (callbackAPFSPayloadFx->gARV_payload_bytes != nullptr && callbackAPFSPayloadFx->gARV_payload_len != nullptr)
	{
		if (*callbackAPFSPayloadFx->gARV_payload_bytes != 0 && *callbackAPFSPayloadFx->gARV_payload_len != 0)
		{
			DBGLOG("APFSPAYLOADFX", "vfs_mountroot can be called, payload and len are correct");
			break;
		}
		DBGLOG("APFSPAYLOADFX", "vfs_mountroot cannot be called, payload = 0x%x, len = 0x%x",
			   *callbackAPFSPayloadFx->gARV_payload_bytes, *callbackAPFSPayloadFx->gARV_payload_len);
		IOSleep(100);
	}
	return FunctionCast(vfs_mountroot, callbackAPFSPayloadFx->org_vfs_mountroot)();
}

//==============================================================================

void APFSPayloadFx::processKernel(KernelPatcher &patcher)
{
	KernelPatcher::RouteRequest requests[] {
		{"_vfs_mountroot", vfs_mountroot, org_vfs_mountroot}
	};
	patcher.routeMultiple(KernelPatcher::KernelID, requests);
	if (patcher.getError() == KernelPatcher::Error::NoError) {
		DBGLOG("APFSPAYLOADFX", "routed %s", requests[0].symbol);
	} else {
		SYSLOG("APFSPAYLOADFX", "failed to resolve %s, error = %d", requests[0].symbol, patcher.getError());
	}
	
	// Ignore all the errors for other processors
	patcher.clearError();
}

//==============================================================================

void APFSPayloadFx::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextList.loadIndex == index) {
		DBGLOG("APFSPAYLOADFX", "%s", kextList.id);
		
		gARV_payload_bytes = reinterpret_cast<int64_t*>(patcher.solveSymbol(index, "_gARV_payload_bytes"));
		if (gARV_payload_bytes == nullptr) {
			SYSLOG("APFSPAYLOADFX", "failed to resolve _gARV_payload_bytes %d", patcher.getError());
			patcher.clearError();
		}
		
		gARV_payload_len = reinterpret_cast<int64_t*>(patcher.solveSymbol(index, "_gARV_payload_len"));
		if (gARV_payload_len == nullptr) {
			SYSLOG("APFSPAYLOADFX", "failed to resolve _gARV_payload_len %d", patcher.getError());
			patcher.clearError();
		}
	}
}
