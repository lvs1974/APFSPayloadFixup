//
//  kern_start.cpp
//  APFSPayloadFixup
//
//  Copyright © 2023 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_apfspayloadfixup.hpp"

static APFSPayloadFx apfs_payload_fx;

static const char *bootargOff[] {
	"-apfspayloadfixupoff"
};

static const char *bootargDebug[] {
	"-apfspayloadfixupdbg"
};

static const char *bootargBeta[] {
	"-apfspayloadfixupbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::Sierra,
	KernelVersion::Ventura,
	[]() {
		apfs_payload_fx.init();
	}
};
