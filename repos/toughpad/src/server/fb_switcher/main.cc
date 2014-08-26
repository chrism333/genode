/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/rpc_server.h>
#include <base/stdint.h>
#include <framebuffer_session/connection.h>
#include <input_session/connection.h>
#include <input/root.h>
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
			
			PINF("refrsh: id = %d, _current_id = %d this=%lx", id, _current_id, this);
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
		int                            _id;

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

/***********************************************
 ** Implementation of the input service **
 ***********************************************/

namespace Input { class Session_component; }

// class Input::Session_component : public Genode::Rpc_object<Session>
// {
// 	private:
// 
// 		Input::Connection &_input;
// 
// 	public:
// 
// 		Session_component(Input::Connection &input)
// 		: _input(input) { }
// 
// 		Genode::Dataspace_capability dataspace() override {
// 			return _input.dataspace(); }
// 
// 		bool is_pending() const override {
// 			return _input.is_pending(); }
// 
// 		int flush() override {
// 			return _input.flush(); }
// 
// 		void sigh(Genode::Signal_context_capability sigh) override {
// 			return _input.sigh( sigh); }
// };


Multiplexer& init_services(Genode::Rpc_entrypoint &ep, Framebuffer::Connection &framebuffer, Input::Connection &input)
{
	using namespace Genode;

	static Input::Session_component input_session;
	static Input::Root_component input_root(ep, input_session);
	
	static Input::Session_component dummy;

	
	static Multiplexer fb_multiplexer(framebuffer, input_session, dummy);
	
	static Framebuffer::Session_component fb_session(fb_multiplexer, 1);
	static Static_root<Framebuffer::Session> fb_root(ep.manage(&fb_session));

	/*
	 * Now, the root interfaces are ready to accept requests.
	 * This is the right time to tell mummy about our services.
	 */
	env()->parent()->announce(ep.manage(&fb_root));
 	env()->parent()->announce(ep.manage(&input_root));
	
	return fb_multiplexer;
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
	
						   
	while (1) {
	  
	  
		unsigned num_events = input.flush();
	  
		for (Input::Event *event = ev_buf; num_events--; event++) {
			if( event->type()  == Input::Event::PRESS && event->code() == 68) // F10 pressed?
				mp.switch_fb();
			else
			    mp.submit(*event);
		}
	}
	return 0;
}