
/* Genode include */
#include <base/env.h>
#include <base/rpc_server.h>
#include <framebuffer_session/framebuffer_session.h>
#include <os/static_root.h>

/* local includes */
#include "services.h"

/***********************************************
 ** Implementation of the framebuffer service **
 ***********************************************/

namespace Framebuffer { class Session_component; }

class Framebuffer::Session_component : public Genode::Rpc_object<Session>
{
	private:

		Framebuffer::Connection &_framebuffer;

	public:

		Session_component(Framebuffer::Connection &framebuffer)
		: _framebuffer(framebuffer) { }

		Genode::Dataspace_capability dataspace() override {
			return _framebuffer.dataspace(); }

		Mode mode() const override {
			return _framebuffer.mode(); }

		void mode_sigh(Genode::Signal_context_capability sigh) override {
			_framebuffer.mode_sigh(sigh); }

		void sync_sigh(Genode::Signal_context_capability sigh) override {
			_framebuffer.sync_sigh(sigh); }

		void refresh(int x, int y, int w, int h) override {
			_framebuffer.refresh(x, y, w, h); }
};

/***********************************************
 ** Implementation of the input service **
 ***********************************************/

namespace Input { class Session_component; }

class Input::Session_component : public Genode::Rpc_object<Session>
{
	private:

		Input::Connection &_input;

	public:

		Session_component(Input::Connection &input)
		: _input(input) { }

		Genode::Dataspace_capability dataspace() override {
			return _input.dataspace(); }

		bool is_pending() const override {
			return _input.is_pending(); }

		int flush() override {
			return _input.flush(); }

		void sigh(Genode::Signal_context_capability sigh) override {
			return _input.sigh( sigh); }
};


void init_services(Genode::Rpc_entrypoint &ep, Framebuffer::Connection &framebuffer, Input::Connection &input)
{
	using namespace Genode;

	static Framebuffer::Session_component fb_session(framebuffer);
	static Static_root<Framebuffer::Session> fb_root(ep.manage(&fb_session));

	static Input::Session_component input_session(input);
	static Static_root<Input::Session> input_root(ep.manage(&input_session));

	/*
	 * Now, the root interfaces are ready to accept requests.
	 * This is the right time to tell mummy about our services.
	 */
	env()->parent()->announce(ep.manage(&fb_root));
 	env()->parent()->announce(ep.manage(&input_root));
}

