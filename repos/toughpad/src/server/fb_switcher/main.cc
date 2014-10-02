/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/rpc_server.h>
#include <base/stdint.h>
#include <framebuffer_session/connection.h>
#include <input_session/connection.h>
#include <input/component.h>
#include <cap_session/connection.h>
#include <input/event.h>
#include <os/static_root.h>
#include <os/attached_ram_dataspace.h>
#include <blit/blit.h>
#include <base/lock.h>
#include <base/lock_guard.h>
#include <base/cancelable_lock.h>

/***********************************************
 ** Implementation of the framebuffer service **
 ***********************************************/

class Multiplexer
{
	private:
		Framebuffer::Connection&       _fb;
		void*                          _target_addr;
		Genode::Attached_ram_dataspace _source_ds_1;
		Genode::Attached_ram_dataspace _source_ds_2;
		void*                          _src_addr_1;
		void*                          _src_addr_2;
		int                            _current_id;
		Input::Session_component&      _input_1;
		Input::Session_component&      _input_2;
		Genode::Lock                   _lock;
		
  
	public:
		Multiplexer(Framebuffer::Connection& fb, Input::Session_component& input_1, Input::Session_component& input_2)
		: _fb(fb),
		  _source_ds_1(Genode::env()->ram_session(), _fb.mode().width()*_fb.mode().height()*_fb.mode().bytes_per_pixel()),
		  _source_ds_2(Genode::env()->ram_session(), _fb.mode().width()*_fb.mode().height()*_fb.mode().bytes_per_pixel()),
		  _current_id(1),
		  _input_1(input_1),
		  _input_2(input_2)
		{
			_target_addr = Genode::env()->rm_session()->attach(_fb.dataspace());
			_src_addr_1  = Genode::env()->rm_session()->attach(_source_ds_1.cap());
			_src_addr_2  = Genode::env()->rm_session()->attach(_source_ds_2.cap());
		}
	  
		Genode::Dataspace_capability dataspace(int id) 
		{
			Genode::Lock::Guard guard(_lock);
			if( id == 1) 
				return _source_ds_1.cap();
			else if( id == 2)
				return _source_ds_2.cap();
			else
				return Genode::Dataspace_capability();
		}

		Framebuffer::Mode mode() const {
			return _fb.mode(); }

		void mode_sigh(Genode::Signal_context_capability sigh, int id) {
			Genode::Lock::Guard guard(_lock);
			if( id == _current_id) _fb.mode_sigh(sigh); }

		void sync_sigh(Genode::Signal_context_capability sigh, int id) {
			Genode::Lock::Guard guard(_lock);
			if( id == _current_id) _fb.sync_sigh(sigh); }

		void refresh(int x, int y, int w, int h, int id) {
			Genode::Lock::Guard guard(_lock);

			if( id != _current_id)
				return;
		
			void* src_addr;
			
			if( id == 1)
				src_addr = _src_addr_1;
			else if( id == 2)
				src_addr = _src_addr_2;
			else
				return;
			
			Framebuffer::Mode mode = _fb.mode();
			
		  
			/* clip specified coordinates against screen boundaries */
			int x2 = Genode::min(x + w - 1, (int)mode.width()  - 1),
				y2 = Genode::min(y + h - 1, (int)mode.height() - 1);
			int x1 = Genode::max(x, 0),
				y1 = Genode::max(y, 0);
			if (x1 > x2 || y1 > y2) return;

			/* determine bytes per pixel */
			int bypp = mode.bytes_per_pixel();
			/* copy pixels from back buffer to physical frame buffer */
			char *src = (char *)src_addr     + bypp*(mode.width()*y + x),
			     *dst = (char *)_target_addr + bypp*(mode.width()*y + x);

			blit(src, bypp*mode.width(), dst, bypp*mode.width(),
				  bypp*(x2 - x1 + 1), y2 - y1 + 1);

			_fb.refresh(x, y, w, h);
		} 
		
		void switch_fb() {
			PINF("switch fb");
			_lock.lock();
			_current_id = _current_id == 1 ? 2 : 1;
			_lock.unlock();
			
			refresh(0, 0, _fb.mode().width(), _fb.mode().height(), _current_id);
		}
		
		void submit(Input::Event& ev) {
			Genode::Lock::Guard guard(_lock);

			if(_current_id == 1)
				_input_1.submit(ev);
			else
				_input_2.submit(ev);
		}
};

namespace Framebuffer { class Session_component; }

class Framebuffer::Session_component : public Genode::Rpc_object<Session>
{
	private:

		Multiplexer&        _mp;
		int                 _id;

	public:

		Session_component(Multiplexer& multiplexer, int id)
		: _mp(multiplexer),
		  _id(id) { }
		  
		Genode::Dataspace_capability dataspace() override {
			return _mp.dataspace(_id); }

		Mode mode() const override {
			return _mp.mode(); }

		void mode_sigh(Genode::Signal_context_capability sigh) override {
			_mp.mode_sigh(sigh, _id); }

		void sync_sigh(Genode::Signal_context_capability sigh) override {
			_mp.sync_sigh(sigh, _id); }

		void refresh(int x, int y, int w, int h) override 
		{
			_mp.refresh(x, y, w, h, _id); 
		}
};

namespace Framebuffer { class Root_component; }

class Framebuffer::Root_component : public Genode::Root_component<Session_component>
{
	private:

		int session_count = 0;
		Session_component& _session1;
		Session_component& _session2;
  
	protected:

		/**
		 * Root component interface
		 */
		Session_component *_create_session(const char *args)
		{
			session_count++;
			if(session_count == 1)
			   return &_session1;
			else if( session_count == 2)
			   return &_session2;
			
			PERR("Invalid session request, fb_switcher provides only two framebuffer sessions!");
			throw Root::Unavailable();
		}

	public:

		Root_component( Genode::Rpc_entrypoint *session_ep, Genode::Allocator *md_alloc, Session_component& session1, Session_component& session2)
		: Genode::Root_component<Session_component>(session_ep, md_alloc), _session1(session1), _session2(session2) { }
};


namespace Input { class Root_component; }

class Input::Root_component : public Genode::Root_component<Session_component>
{
	private:

		int session_count = 0;
		Session_component& _session1;
		Session_component& _session2;
  
	protected:

		/**
		 * Root component interface
		 */
		Session_component *_create_session(const char *args)
		{
			session_count++;
			if(session_count == 1)
			{
				_session1.event_queue().enabled(true);
				return &_session1;
			}
			else if( session_count == 2)
			{
				_session2.event_queue().enabled(true);
				return &_session2;
			}
			
			PERR("Invalid session request, fb_switcher provides only two input sessions!");
			throw Root::Unavailable();
		}

	public:

		Root_component( Genode::Rpc_entrypoint *session_ep, Genode::Allocator *md_alloc, Session_component& session1, Session_component& session2)
		: Genode::Root_component<Session_component>(session_ep, md_alloc), _session1(session1), _session2(session2) { }
};


Multiplexer& init_services(Genode::Rpc_entrypoint &ep, Framebuffer::Connection &framebuffer, Input::Connection &input)
{
	using namespace Genode;

	static Input::Session_component input_session1;
	static Input::Session_component input_session2;
	static Input::Root_component input_root(&ep, Genode::env()->heap(), input_session1, input_session2);

	static Multiplexer multiplexer(framebuffer, input_session1, input_session2);
	static Framebuffer::Session_component fb_session1(multiplexer, 1);
	static Framebuffer::Session_component fb_session2(multiplexer, 2);
	
	static Framebuffer::Root_component fb_root(&ep, Genode::env()->heap(), fb_session1, fb_session2);

	/*
	 * Now, the root interfaces are ready to accept requests.
	 * This is the right time to tell mummy about our services.
	 */
	env()->parent()->announce(ep.manage(&fb_root));
	env()->parent()->announce(ep.manage(&input_root));

	return multiplexer;
}


int main(int, char **)
{
	using namespace Genode;

	PDBG("--- fb_switcher service started ---");

	static Framebuffer::Connection framebuffer;
	static Input::Connection       input;
	static Cap_connection          cap;

	Dataspace_capability ev_ds_cap = input.dataspace();

	Input::Event *ev_buf = static_cast<Input::Event *>
	                       (env()->rm_session()->attach(ev_ds_cap));

	/* initialize server entry point */
	enum { STACK_SIZE = 2*1024*sizeof(Genode::addr_t) };
	static Genode::Rpc_entrypoint ep(&cap, STACK_SIZE, "fb_switcher_ep");

	/* initialize public services */
	Multiplexer& mp = init_services(ep, framebuffer, input);
   
	int key_count = 0;
	
	while (1) { 
		unsigned num_events = input.flush();
	  
		for (Input::Event *event = ev_buf; num_events--; event++) {

			// keep track of number of pressed keyes
			if( event->type()  == Input::Event::PRESS)
				key_count++;
			else if( event->type()  == Input::Event::RELEASE)
				key_count--;
		  
			if( event->type()  == Input::Event::PRESS && event->code() == 68 && key_count == 1) // F10 pressed and are all other buttons released?
				mp.switch_fb();
			else
			    mp.submit(*event);
		}
	}
	return 0;
}