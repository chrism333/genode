/*
 * \brief  VirtualBox usb device models
 * \author Christian Menard <christian.menard@ksyslabs.org>
 * \date   2014-08-1
 */

/*
 * Copyright (C) 2014 Ksys Labs LLC
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/printf.h>

/* VirtualBox includes */
#include <VBoxDD.h>
#include <VBoxDD2.h>


extern "C" int VBoxUsbRegister(PCPDMUSBREGCB pCallbacks, uint32_t u32Version)
{
	PDBG("VBoxUsbRegister called");

	int rc = 0;

	rc = pCallbacks->pfnRegister(pCallbacks, &g_UsbHidMou);
		if (RT_FAILURE(rc))
			return rc;

	return rc;
}
