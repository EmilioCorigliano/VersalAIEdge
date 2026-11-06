/**<!--
* Copyright (C) 2024 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
* -->
*/
#ifndef AIE_BAREMETAL_LIB_H
#define AIE_BAREMETAL_LIB_H
#include <iostream>
#include <unistd.h>
#include <memory>
#include "common_layer/aeg_api_message.h"
using namespace abr;
#define GRAPH_COMPILE_OR_SIM (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#if GRAPH_COMPILE_OR_SIM
#define AbrGraph BaremetalGraphSim
#define AIEGRAPH AIEGraphSim
typedef int ERR_CODE;
#else
#define AbrGraph BaremetalGraph
#define AIEGRAPH AIEGraph
#endif

#include "aeg_baremetal_api.h"

//TODO: remove this hardcode after aiecompiler add rpu base address into json
#define RPU_AIE_BASE 0x40000000
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include <dlfcn.h>
#include <stdio.h>
#define CREATEGRAPHAPI "createBaremetalGraphInstance"
#define AIE_BAREMETAL_LIB "AIE_BAREMETAL_LIB"
class BaremetalGraphSimBase {
public:
	virtual void selftestexit()=0;
	virtual ERR_CODE clock_enable()=0;
	virtual ERR_CODE clock_disable()=0;
	virtual void error_handling_cb_init(void *cb,
				    void *priv)=0;
	virtual void init()=0;
	virtual void run(int num)=0;
	virtual void gm2aie_nb(char* name, char* data, int len)=0;
	virtual void aie2gm_nb(char* name, char* data, int len)=0;
	virtual void gm2aie(char* name, char* data, int len)=0;
	virtual void aie2gm(char* name, char* data, int len)=0;
	virtual void wait()=0;

	// External Buffer APIs
	virtual void setAddress(char* externalbuffer_name,  void* ptr)=0;
	virtual ERR_CODE wait(char* externalbuffer_name, std::string port_name)=0;
	virtual ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name)=0;
	virtual ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name)=0;
	virtual ERR_CODE setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2)=0;
	// End external buffer APIs

	virtual ERR_CODE end()=0;
	virtual ERR_CODE gmio_wait(std::string gmio_name,bool syncmode=true)=0;
	virtual void* malloc(uint32_t len)=0;
	virtual void free(void* addr)=0;
	
	virtual ERR_CODE update(const std::string& port_name, void* data, size_t len)=0;
	virtual ERR_CODE read(const std::string& port_name, char* data, int len)=0;
	virtual void* BacktrackErrors()=0;
	virtual void gdbpoint()=0;

  virtual int start_profiling(const std::string& port_name, int option, uint32_t value = 0)=0;
  virtual int start_profiling(const std::string& port_name1,
															 	  const std::string& port_name2,
															    int option,
															    uint32_t value = 0)=0;
  virtual long long read_profiling(int h)=0;
  virtual void stop_profiling(int h)=0;
};

typedef BaremetalGraphSimBase* (*createBaremetalGraphInstance_t)(const char* name, const uint8_t rpu);

class BaremetalGraphSim {
public:
	BaremetalGraphSim(std::string GraphName, uint8_t rpu=0) {
#if !defined(__ADF_FRONTEND__)
		bgraph = createBGraphInstance(GraphName, rpu);
#endif
	}
	~BaremetalGraphSim() {
#if !defined(__ADF_FRONTEND__)
		if (bgraph) {
			delete bgraph;
		}
		if (handle) {
			dlclose(handle);
		}
#endif
	}

	void selftestexit() {
		if (bgraph) {
			bgraph->selftestexit();
		} else {
		}
	}

	ERR_CODE clock_enable() {
		if (bgraph) {
			return bgraph->clock_enable();
		} else {
			return -1;
		}
	}

	ERR_CODE clock_disable() {
		if (bgraph) {
			return bgraph->clock_disable();
		} else {
			return -1;
		}
	}

	void error_handling_cb_init(void *cb, void *priv){
		if (bgraph) {
			bgraph->error_handling_cb_init(cb, priv);
		} else {
		}
	}

	void init() {
		if (bgraph) {
			bgraph->init();
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	void run(int num=-1) {
		if (bgraph) {
			bgraph->run(num);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	void gm2aie_nb(char* name, char* data, int len) {
		if (bgraph) {
			bgraph->gm2aie_nb(name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	void aie2gm_nb(char* name, char* data, int len) {
		if (bgraph) {
			bgraph->aie2gm_nb(name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	void gm2aie(char* name, char* data, int len) {
		if (bgraph) {
			bgraph->gm2aie(name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	void aie2gm(char* name, char* data, int len) {
		if (bgraph) {
			bgraph->aie2gm(name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	ERR_CODE gmio_wait(std::string gmio_name,bool syncmode=true) {
		int ret = 0;
		if (bgraph) {
			ret = bgraph->gmio_wait(gmio_name, syncmode);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
		return ret;
	}

    // External Buffer APIs
	void setAddress(char* externalbuffer_name, void* ptr){
		if (bgraph) {
			bgraph->setAddress(externalbuffer_name, ptr);
		} else {
			std::cout << "External Buffer setaddress failed " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}

	ERR_CODE wait(char* externalbuffer_name, std::string port_name){
		if (bgraph) {
			bgraph->wait(externalbuffer_name, port_name);
		} else {
			std::cout << "External Buffer wait failed " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return (ERR_CODE)err_code::user_error;
		}
		return (ERR_CODE)err_code::ok;
	}

	ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name){
		if (bgraph) {
			bgraph->gm2aie_nb(externalbuffer_name, port_name);
		} else {
			std::cout << "External Buffer gm2aie_nb failed " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return (ERR_CODE)err_code::user_error;
		}
		return (ERR_CODE)err_code::ok;
	}

	ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name){
		if (bgraph) {
			bgraph->aie2gm_nb(externalbuffer_name, port_name);
		} else {
			std::cout << "External Buffer aie2gm_nb failed " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return (ERR_CODE)err_code::user_error;
		}
		return (ERR_CODE)err_code::ok;
	}

	ERR_CODE setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2){
		if (bgraph) {
			bgraph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
		} else {
			std::cout << "External Buffer setupPingPongBuffers failed " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return (ERR_CODE)err_code::user_error;
		}
		return (ERR_CODE)err_code::ok;
	}
	// End external buffer APIs

	void wait() {
		if (bgraph) {
			bgraph->wait();
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}
	
	err_code end() {
		if (bgraph) {
			return (err_code)bgraph->end();
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return err_code::user_error;
		}
		return err_code::ok;
	}

	void* malloc(uint32_t len) {
		if (bgraph) {
			return bgraph->malloc(len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return NULL;
		}
	}

	void free(void* addr) {
		if (bgraph) {
			bgraph->free(addr);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
	}
	
	err_code update(const std::string& port_name, void* data, size_t len)
	{
		if (bgraph) {
			return (err_code)bgraph->update(port_name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return err_code::user_error;
		}
	}

	err_code read(const std::string& port_name, char* data, int len)
	{
		if (bgraph) {
			return (err_code)bgraph->read(port_name, data, len);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return err_code::user_error;
		}
	}

	void* BacktrackErrors() {
		if (bgraph) {
			return bgraph->BacktrackErrors();
		} else {

			std::cout << "baremetal graph error backtracking failed, please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return NULL;
		}
	}

	void gdbpoint() {
		if (bgraph) {
			return bgraph->gdbpoint();
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return;
		}
	}

  	int start_profiling(const std::string& port_name, int option, uint32_t value = 0){
		if (bgraph) {
			return bgraph->start_profiling(port_name, option, value);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return 0;
		}
	}

  	int start_profiling(const std::string& port_name1,
															 	  const std::string& port_name2,
															    int option,
															    uint32_t value = 0) {
		if (bgraph) {
			return bgraph->start_profiling(port_name1, port_name2, option, value);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return 0;
		}
	}

  long long read_profiling(int h) {
		if (bgraph) {
			return bgraph->read_profiling(h);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
			return 0;
		}
	}
	
  void stop_profiling(int h) {
		if (bgraph) {
			bgraph->stop_profiling(h);
		} else {
			std::cout << "baremetal graph load failed ,please check the " ;
			std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
		}
		return;
	}

private:
	void* handle = NULL;
	BaremetalGraphSimBase* bgraph;

	BaremetalGraphSimBase* createBGraphInstance(std::string gname, uint8_t rpu) {
		const char* ablib = getenv(AIE_BAREMETAL_LIB);
		if (!ablib) {
			printf("AIE_BAREMETAL_LIB environment variable not set\n");
			return nullptr;
		}
		handle = dlopen(ablib, RTLD_LAZY);

		if (handle != NULL) {

			createBaremetalGraphInstance_t createInstance = (createBaremetalGraphInstance_t) dlsym(handle, CREATEGRAPHAPI);
			if (!createInstance) {
				printf("Cannot load symbol: \n");
				dlclose(handle);
				return nullptr;
			}
			
			return createInstance(gname.c_str(), rpu);
		}
		const char* error = dlerror();
    if (error) {
            std::cout << "Error loading library: " << error << std::endl;
    } else {
            std::cout << "Unknown error loading library." << std::endl;
    }
		printf("load failed !\n");
		return nullptr;
	}
};
#endif
#endif // AIE_BAREMETAL_LIB_H
 
