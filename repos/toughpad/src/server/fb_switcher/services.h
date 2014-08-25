/*
 * \brief  Fb_nit-internal service interface
 * \author Norman Feske
 * \date   2006-09-22
 */

/*
 * Copyright (C) 2006-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SERVICES_H_
#define _SERVICES_H_

#include <base/rpc_server.h>
#include <framebuffer_session/connection.h>
#include <input_session/connection.h>

extern void init_services(Genode::Rpc_entrypoint &ep, Framebuffer::Connection& framebuffer, Input::Connection &input);

#endif
