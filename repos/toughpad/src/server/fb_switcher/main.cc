/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <framebuffer_session/connection.h>
#include <input_session/connection.h>
#include <cap_session/connection.h>
#include <input/event.h>

#include "services.h"


 
int main(int, char **)
{
	using namespace Genode;

	PDBG("--- fb_switcher service started ---");

	static Framebuffer::Connection framebuffer;
	static Input::Connection       input;
	static Cap_connection          cap;

// 	Dataspace_capability ev_ds_cap = input.dataspace();
// 
// 	Input::Event *ev_buf = static_cast<Input::Event *>
// 	                       (env()->rm_session()->attach(ev_ds_cap));
// 
// 	Dataspace_capability fb_ds_cap = framebuffer.dataspace();
// 
// 	void* fb_addr = Genode::env()->rm_session()->attach(fb_ds_cap);
// 	
// 	Framebuffer::Mode fb_mode = framebuffer.mode();
				
	
	/* initialize server entry point */
	enum { STACK_SIZE = 2*1024*sizeof(Genode::addr_t) };
	static Genode::Rpc_entrypoint ep(&cap, STACK_SIZE, "fb_switcher_ep");
	
	/* initialize public services */
	init_services(ep, framebuffer, input);
	
	int i = 0;
						   
	while (1) {
	  
	  
// 		unsigned num_events = input.flush();
// 	  
// 		for (Input::Event *event = ev_buf; num_events--; event++) {
// 			PDBG("input event");
// 		  
// 			for( int j=0; j < 1000; j++)
// 			{
// 				*((uint8_t*)(fb_addr + i)) = 0xff;
// 				i++;
// 			}
// 			
// 			framebuffer.refresh(0, 0, fb_mode.width(), fb_mode.height());
// 		}
	}
	return 0;
}