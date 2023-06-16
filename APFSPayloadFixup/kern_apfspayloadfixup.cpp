//
//  kern_apfspayloadfixup.cpp
//  APFSPayloadFixup
//
//  Copyright © 2023 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_file.hpp>

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
	if (callbackAPFSPayloadFx->gARV_payload_bytes == nullptr)
	{
		return FunctionCast(vfs_mountroot, callbackAPFSPayloadFx->org_vfs_mountroot)();
	}
	
	DBGLOG("APFSPAYLOADFX", "vfs_mountroot is called");
	PANIC_COND(*callbackAPFSPayloadFx->gARV_payload_bytes == 0, "APFSPAYLOADFX", "vfs_mountroot: APFS payload pointer is nullptr");
	
	return FunctionCast(vfs_mountroot, callbackAPFSPayloadFx->org_vfs_mountroot)();
}

//==============================================================================

bool APFSPayloadFx::AppleAPFSContainer_start(IOService* service, IOService* provider)
{
	DBGLOG("APFSPAYLOADFX", "AppleAPFSContainer start is called, service name is %s, provider name is %s",
		   safeString(service->getName()), safeString(provider->getName()));
	IOSleep(3000);
	return FunctionCast(AppleAPFSContainer_start, callbackAPFSPayloadFx->orgAppleAPFSContainer_start)(service, provider);
}

//==============================================================================

int64_t APFSPayloadFx::external_module_resources()
{
	if (callbackAPFSPayloadFx->gARV_payload_bytes == nullptr || callbackAPFSPayloadFx->gARV_payload_len == nullptr)
	{
		return FunctionCast(external_module_resources, callbackAPFSPayloadFx->org_external_module_resources)();
	}
			
	DBGLOG("APFSPAYLOADFX", "external_module_resources is called, before: payload = 0x%llx, len = 0x%llx",
		   *callbackAPFSPayloadFx->gARV_payload_bytes, *callbackAPFSPayloadFx->gARV_payload_len);
	auto result = FunctionCast(external_module_resources, callbackAPFSPayloadFx->org_external_module_resources)();
	DBGLOG("APFSPAYLOADFX", "external_module_resources, after: result = 0x%llx, payload = 0x%llx, len = 0x%llx", result,
		   *callbackAPFSPayloadFx->gARV_payload_bytes, *callbackAPFSPayloadFx->gARV_payload_len);
	
	if (*callbackAPFSPayloadFx->gARV_payload_bytes != 0 && *callbackAPFSPayloadFx->gARV_payload_len != 0)
	{
		callbackAPFSPayloadFx->last_used_payload_bytes = *callbackAPFSPayloadFx->gARV_payload_bytes;
		callbackAPFSPayloadFx->last_payload_len = *callbackAPFSPayloadFx->gARV_payload_len;
	}
	
	if (*callbackAPFSPayloadFx->gARV_payload_bytes == 0)
	{
		for (int attempts = 20; --attempts >= 0;)
		{
			if (*callbackAPFSPayloadFx->gARV_payload_bytes != 0)
			{
				DBGLOG("APFSPAYLOADFX", "external_module_resources returned correct data!");
				break;
			}
			SYSLOG("APFSPAYLOADFX", "payload or len are not correct, try to call external_module_resources");
			auto result = FunctionCast(external_module_resources, callbackAPFSPayloadFx->org_external_module_resources)();
			DBGLOG("APFSPAYLOADFX", "external_module_resources result = 0x%llx, payload = 0x%llx, len = 0x%llx", result,
				   *callbackAPFSPayloadFx->gARV_payload_bytes, *callbackAPFSPayloadFx->gARV_payload_len);
			IOSleep(100);
		}
		
		if (*callbackAPFSPayloadFx->gARV_payload_bytes == 0)
		{
			if (callbackAPFSPayloadFx->last_used_payload_bytes != 0)
			{
				SYSLOG("APFSPAYLOADFX", "payload or len are not correct, restore from last_used_payload_bytes and last_payload_len");
				*callbackAPFSPayloadFx->gARV_payload_bytes = callbackAPFSPayloadFx->last_used_payload_bytes;
				*callbackAPFSPayloadFx->gARV_payload_len = callbackAPFSPayloadFx->last_payload_len;
			}
			else
			{
				PANIC_COND(*callbackAPFSPayloadFx->gARV_payload_bytes == 0, "APFSPAYLOADFX", "apfs_kext: payload or len are not correct, nothing can be done");
			}
		}
	}
	
	return result;
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
		
		KernelPatcher::RouteRequest requests[] {
			{"__ZN18AppleAPFSContainer5startEP9IOService", AppleAPFSContainer_start, orgAppleAPFSContainer_start},
			{"_external_module_resources", external_module_resources, org_external_module_resources}};
		
		patcher.routeMultiple(index, requests, address, size);
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("APFSPAYLOADFX", "routed %s", requests[0].symbol);
		} else {
			SYSLOG("APFSPAYLOADFX", "failed to resolve %s, error = %d", requests[0].symbol, patcher.getError());
			patcher.clearError();
		}
	}
}
