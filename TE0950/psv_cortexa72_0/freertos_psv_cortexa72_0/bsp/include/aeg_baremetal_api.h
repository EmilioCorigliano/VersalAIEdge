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
#ifndef AIE_BAREMETAL_API_H
#define AIE_BAREMETAL_API_H
#include <iostream>
#include <vector>
#include <unistd.h>
#include <memory>

#define PS_SO "ps_i"

typedef int ERR_CODE;
typedef int PROFILE_ID;

class AIEGraphSim;
class AIEGraph;
using BaremetalGraph = AIEGraph;

class AIEGraph {
public:
	AIEGraph(std::string GraphName);
	~AIEGraph();
	void* malloc(size_t len);
	void free(void* mem);
	void selftestexit();
	ERR_CODE clock_enable();
	ERR_CODE clock_disable();

	void error_handling_cb_init(void *cb, void *priv);
	void init();
	void run(int num=-1);
	void gm2aie_nb(char* name, char* data, int len) ;
	void aie2gm_nb(char* name, char* data, int len);
	void gm2aie(char* name, char* data, int len) ;
	void aie2gm(char* name, char* data, int len);
	void wait();
	ERR_CODE end();
	ERR_CODE gmio_wait(std::string gmio_name,bool syncmode=true);
	ERR_CODE update(const std::string& port_name, void* data, size_t len);
	ERR_CODE read(const std::string& port_name, char* data, int len);

	// get the aie error report after doing the backtrack.
	void* BacktrackErrors();

	// External Buffer APIs
	void setAddress(char* externalbuffer_name, void* ptr);
	ERR_CODE wait(char* externalbuffer_name, std::string port_name);
	ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name);
	ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name);
	ERR_CODE setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2);

	// Profiling APIs
    PROFILE_ID start_profiling(const std::string& port_name, int option, uint32_t value = 0);
    PROFILE_ID start_profiling(const std::string& port_name1,
	                           const std::string& port_name2,
					           int option, uint32_t value = 0);
    long long read_profiling(PROFILE_ID h);
    void stop_profiling(PROFILE_ID h);

private:
	std::shared_ptr<AIEGraphSim> graph;
};

enum options_enum_init
{
  io_profiling_option_enum_init = 0x20
};

enum io_profiling_option
{
  io_total_stream_running_to_idle_cycles = io_profiling_option_enum_init, /// Total accumulated cycles in between stream running to stream idle events
  io_stream_start_to_bytes_transferred_cycles,                            /// Cycles in between the first event of stream running to the event that the specified number of bytes are transferred (assuming the stream stops right after the specified number of bytes are transferred)
  io_stream_start_difference_cycles,                                      /// Cycles elapsed between the first stream running events of the two platform IO objects
  io_stream_running_event_count                                           /// Number of stream running events
};

#endif // AIE_BAREMETAL_API_H
 
