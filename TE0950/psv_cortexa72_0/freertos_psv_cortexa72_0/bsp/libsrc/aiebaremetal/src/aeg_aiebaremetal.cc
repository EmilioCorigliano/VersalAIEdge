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
#include <iostream>
#include <unordered_map>
#include <vector>
#ifdef DEBUG_RUNTIME
#include "hw_config.h"
#endif
//include mpu header
#ifdef ARMR5
#include "xil_mpu.h"
#endif

#include <xaiengine.h>
#include <sstream>
#include "aiebaremetal.h"
#include "aeg_configure.h"
#include "xaiemem.h"
#include <unistd.h>
#include "log/klogr.h"
#include "event.h"
#include "xaie_generate_events.h"
#include "cert_host.h"
#include "cert_elf_loader.h"

#ifdef __AIESIM__
#include "drivers/aiengine/xioutils.h"
#include "aiesim_base_apis.h"
#include <dlfcn.h>
#endif

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include <xil_exception.h>
#include <xstatus.h>
#include "xparameters.h"
#include "xparameters_ps.h"	/* defines XPAR values */
#include "xil_cache.h"
#include "xscugic.h"
#include "xil_printf.h"
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

using namespace abr;
std::vector<std::shared_ptr<AIEGraphSim>> *AIEGraphSim_List = NULL;

#include "common_layer/aeg_api_message.h"
#include "common_layer/aegothers/fal_util.h"

extern unsigned char ctrl_code_elf[];
extern unsigned int ctrl_code_elf_len;

#ifdef __AIESIM__
extern void* ess_Write32_Ptr;
extern void* ess_Read32_Ptr;

extern int g_start_plio;
extern const char _binary_scsim_config_json_start[];
extern const char _binary_scsim_config_json_end[];

auto len = _binary_scsim_config_json_end - \
				_binary_scsim_config_json_start;
std::string json_file(_binary_scsim_config_json_start, len);

const char* getLibPathfromScsimConfig(std::string& json_file) {
	std::istringstream iss(json_file);
	std::ofstream outfile("temp.txt", std::ios::out);
	outfile << iss.str();
	outfile.close();

	boost::property_tree::ptree pt;
	std::string short_lib_path;

	boost::property_tree::read_json(iss, pt);
	for (auto& child : pt.get_child("SimulationConfig.pl_ip_block")) {
		short_lib_path = child.second.get<std::string>("lib_path");
	}
	const char *work_path = "./Work/";//getenv("AIE_COMPILE_WORK");
	size_t len = std::strlen(work_path) + std::strlen(short_lib_path.c_str()) + 1;
	char* lib_path = new char[len];
	std::sprintf(lib_path, "%s%s", work_path, short_lib_path.c_str());
	return lib_path;
}

const char* lib_path = getLibPathfromScsimConfig(json_file);

static void open_dl(const char* lib_path) {
	void* handle = dlopen(lib_path, RTLD_LAZY);
	if (!handle) {
		throw std::runtime_error(dlerror());
	}
	ess_Write32_Ptr = (void*)dlsym(handle, "ess_Write32");
	ess_Read32_Ptr = (void*)dlsym(handle, "ess_Read32");

	return;
}

#endif

extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
XScuGic xInterruptController;
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */

void abr_error_isr(void *data);
static void abr_init_isr()
{
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
	static int init_isr = 0;
	int Ret = 0;
	XScuGic_Config *IntcConfig;
#define AIE_IRQ_VECT_ID_0		180U

	if (init_isr) {
		return;
	}
	Xil_ExceptionInit();
	XScuGic_DeviceInitialize(INTC_DEVICE_ID);

	Xil_ExceptionDisable();	/* Initialize the interrupt controller driver */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig)
		return;
	Ret = XScuGic_CfgInitialize(&xInterruptController, IntcConfig,
				    IntcConfig->CpuBaseAddress);
	if (Ret != XST_SUCCESS)
		return;
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_DeviceInterruptHandler,
			(void *)INTC_DEVICE_ID);

	Xil_ExceptionEnable();
	XScuGic_Connect(&xInterruptController, AIE_IRQ_VECT_ID_0,
			(Xil_InterruptHandler) abr_error_isr, (void *) NULL);

	XScuGic_Enable(&xInterruptController, AIE_IRQ_VECT_ID_0);

	init_isr = 1;
	return;
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */
}

void dump_uc_app() {
#ifdef __AIESIM__
	std::ofstream outFile("output.bin", std::ios::binary | std::ios::out);
	if (!outFile) {
		std::cerr << "Failed to open file for writing." << std::endl;
		return;
	}
	// Read data from istringstream and write it to the binary file
	outFile.write(_binary___app_elf_start, _binary___app_elf_end - _binary___app_elf_start);
	// Close the file
	outFile.close();
#endif
}

void aie_driver_test(XAie_DevInst *DevInst) {
	(void)DevInst;
}
class AIEGraphSim {
public:
	~AIEGraphSim() {
		XAie_Finish(&DevInst);
	}
	AIEGraphSim(std::string GraphName) {
		int rpu = 0;
		#ifdef ARMR5
		rpu = 1;
		#endif
		struct abr::aiecompiler_options aiecompiler_options;
		
		auto& aiecompilerconfig = cfg.get_aiecompiler_config();
		aiecompiler_options.broadcast_enable_core = aiecompilerconfig.broadcast_enable_core;
		xaiemem = std::make_shared<XAieMem>(&DevInst);
		auto& dconfig = cfg.get_driver_config();
		auto& gconfig = cfg.get_graph_config(GraphName);
		GraphApi = std::make_shared<abr::graph_api>(&gconfig);
		EventApi = std::make_shared<abr::event>(&cfg);
		auto aiebase = dconfig.base_address;
		//TODO: AIE compiler should provide a rpu base_address, after that change the logic, now just hardcode.
		if (rpu) {
			aiebase = RPU_AIE_BASE;
		}
		XAie_SetupConfig(ConfigPtr,
				dconfig.hw_gen,
				aiebase,//dconfig.base_address,
				dconfig.column_shift,
				dconfig.row_shift,
				dconfig.num_columns,
				dconfig.num_rows,
				dconfig.shim_row,
				dconfig.mem_row_start,
				dconfig.mem_num_rows,
				dconfig.aie_tile_row_start,
				dconfig.aie_tile_num_rows);


		AieRC RC = XAie_SetupPartitionConfig(&DevInst, (aiebase + (dconfig.partition_overlay_start_cols[0] << dconfig.column_shift)),
		 dconfig.partition_overlay_start_cols[0],
		 dconfig.partition_num_cols);

		if(RC != XAIE_OK) {
			printf("Partition initialization failed.\n");
			return;
		}
		
		RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
		if(RC != XAIE_OK) {
			printf("Driver initialization failed.\n");
			return;
		}
		fal_util::initialize(&DevInst);
		abr::config_manager::initialize(&DevInst, dconfig.mem_num_rows, aiecompiler_options.broadcast_enable_core);
		this->cert.init_devinst(&this->DevInst);
		GraphApi->configure();
		#ifdef __AIESIM__
		#ifndef __LOCAL_TEST__
		open_dl(lib_path);
		#endif
		#endif
	}
	
	/**
	 * @brief Test some functions in this file. 
	 * @note  Updates NPI address based on simulation mode, tests some clock functions.
	 */
	void selftestexit() {
		int ret;

#ifdef __AIESIM__
		XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_SIM);
		switch (DevInst.DevProp.DevGen) {
		case XAIE_DEV_GEN_AIE2PS:
		case XAIE_DEV_GEN_AIEML:
			XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
			break;
		case XAIE_DEV_GEN_AIE:
			XAie_UpdateNpiAddr(&DevInst, 0xF70A0000);
			break;
		default:
			printf("DevGen not found: %d\n", DevInst.DevProp.DevGen);
		}
#endif /* __AIESIM__ */
		ret = selftest_errorhandling();

		ret = clock_enable();
		if (ret) {
			printf("Clock enable failed\n");
		} else {
			printf("Clock enabled.\n");
		}
		ret = clock_disable();
		if (ret) {
			printf("Clock disable failed\n");
		} else {
			printf("Clock disabled.\n");
		}
		ret = clock_enable();
		if (ret) {
			printf("Clock enable failed\n");
		} else {
			printf("Clock enabled.\n");
		}

	}

	/**
	 * @brief Enable clock.
	 * @return Error code. 0 is failure, 1 is true. 
	 */
	ERR_CODE clock_enable() {
		return XAie_PmSetColumnClk(&DevInst, DevInst.StartCol, DevInst.NumCols, 1);
	}
	
	/**
	 * @brief Disable clock.
	 * @return Error code. 0 is failure, 1 is true. 
	 */
	ERR_CODE clock_disable() {
		return XAie_PmSetColumnClk(&DevInst, DevInst.StartCol, DevInst.NumCols, 0);
	}

	/**
	 * @brief	Sets up callback function for error handling.
	 * @param	cb		Pointer to the callback function. 
	 * @param	priv	Used to pass any information to error handling function.
	 */
	void error_handling_cb_init(void *cb,
				    void *priv) {
		error_handling_cb = (void (*)(std::vector<XAie_ErrorPayload> , void *))cb;
		error_handling_cb_priv = priv;
	}

	/**
	 * @brief Configures GMIOS and External Buffers.
	 */
	void init() {
		uint8_t loop = 0;
		while(loop) {
			std::cout << "init - pid = " << getpid() <<std::endl;
			sleep(1);
		}
//support enable RPU cache by set aie memory attribute
#ifdef ARMR5
		Xil_SetMPURegion(RPU_AIE_BASE, 0x40000000, DEVICE_NONSHARED | PRIV_RW_USER_RW);
#endif
		cfg.ConfigureGmios(xaiemem);
		cfg.ConfigureExternalBuffers(xaiemem);
	}

	/**
	 * @brief  Starts and runs a graph for num iterations.
	 * @note   Different modes, aiesim and debug.
	 * @param  num       Number of iterations. 
	 */
	void run(int num) {

#ifdef __AIESIM__
		aiesim::StartPlios();
#endif
		GraphApi->run(num);
	}

	/**
	 * @brief  Grabs a GMIO and use it to transfer data from AIE to GMIO. 
	 * @note   This version proceeds even if there is an error. 
	 * @param  gmio_name   Used to find which GMIO api implementation to use.
	 * @param  data        The data being transferred from AIE to GMIO.
	 * @param  len         Length of data.
	 */
	void aie2gm_nb(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->aie2gm_nb(data, len);
	}

	/**
	 * @brief  Grabs a GMIO and use it to transfer data from AIE to GMIO. 
	 * @note   This version waits if there is an error. 
	 * @param  gmio_name   Used to find which GMIO api implementation to use.
	 * @param  data        The data being transferred from AIE to GMIO.
	 * @param  len         Length of data.
	 */
	void aie2gm(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->aie2gm(data, len);
	}

#ifdef DEBUG_RUNTIME
	void gm2aie_check() {
		int channel = 0, col = 12, row = 0;
		XAie_LocType loc = XAie_TileLoc(12, 0);
		const uint32_t col_12_row_0 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
		uint32_t  mm2sc0_status = 0x00009328;
		uint32_t d1;
		int n = 1;

		while(n-- >= 0) {
			XAie_Read32(&DevInst, col_12_row_0 + mm2sc0_status, &d1);
			std::cout << "mm2s status " << d1 << std::endl;
			usleep(10);
		}
   
	}
#endif

	/**
	 * @brief  Grabs a GMIO and use it to transfer data from GMIO to AIE. 
	 * @note   This version proceeds even if there is an error. 
	 * @param  gmio_name   Used to find which GMIO api implementation to use.
	 * @param  data        The data being transferred from GMIO to AIE.
	 * @param  len         Length of data.
	 */
	void gm2aie_nb(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->gm2aie_nb(data, len);
		//gm2aie_check();
	}

	/**
	 * @brief  Grab a GMIO and use it to transfer data from GMIO to AIE. 
	 * @note   This version waits if theres an error.
	 * @param  gmio_name   Used to find which GMIO api implementation to use.
	 * @param  data        The data being transferred from GMIO to AIE.
	 * @param  len         Length of data.
	 */
	void gm2aie(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name); //grab gmio api impl based on gmio name 
		gmio_api_impl->gm2aie(data, len); // use the gmio api impl version of gm2aie to map data from gmio to aie 
	}

	// External Buffer APIs

	/**
	 * @brief  Set address of pointer within external buffer API.
	 * @param  externalbuffer_name   Used to find which external buffer api implementation to use.
	 * @param  ptr                   The address of the pointer.
	 */
	void setAddress(char* externalbuffer_name, void* ptr){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->setAddress(ptr);
	}

	/**
	 * @brief  Call wait function using external buffer API.
	 * @param  externalbuffer_name   Used to find which external buffer api implementation to use.
	 * @param  port_name             The port that this function is for.
	 * @return Returns 0 on resolution, error code will be found inside external buffer.
	 */
	ERR_CODE wait(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->wait(port_name);

		return 0;
	}

	/**
	 * @brief  Call gm2aie_nb function using external buffer API.
	 * @note   External buffer api accesses data using tiling parameters. Supported on AI Engine-ML devices. 
	 * @param  externalbuffer_name   Used to find which external buffer api implementation to use.
	 * @param  port_name             The port that this function is for.
	 * @return  Returns 0 on resolution, error code will be found inside external buffer.
	 */
	ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->gm2aie_nb(port_name);

		return 0;
	}

	/**
	 * @brief  Call aie2gm_nb function using external buffer API.
	 * @note   External buffer api accesses data using tiling parameters. Supported on AI Engine-ML devices. 
	 * @param  externalbuffer_name   Used to find which external buffer api implementation to use.
	 * @param  port_name             The port that this function is for.
	 * @return  Returns 0 on resolution, error code will be found inside external buffer.
	 */
	ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->aie2gm_nb(port_name);

		return 0;
	}

	/**
	 * @brief Sets up a ping pong buffer system.
	 * @note  Uses external buffers for data transfer. 
	 * @param externalbuffer_name     Name of external buffer to be used.
	 * @param ptr1                    Address of 1st pointer. 
	 * @param ptr2                    Address of 2nd pointer.
	 * @return     					  An error code indicating result of the call. 
	 */
	ERR_CODE setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		return (ERR_CODE)externalbuffer_api_impl->setupPingPongBuffers(ptr1, ptr2);
}

    // End external Buffer APIs

		/**
	 * @brief Calls update function for Remote Tile Processing (RTP) or Shared Buffer configuration corresponding to the port type.
	 * @note  Checks for errors, manages sync locks and reading and writing data.
	 * @param port_name		Configured RTP or Shared Buffer Name.  
	 * @param data			Data that needs to be updated.
	 * @param len			Num of bytes of data.
	 * @return 	An error code. err_code::ok if opertaion successful, 
	 * 			err_code::aie_driver_error and message if error.
	 */
	ERR_CODE update(const std::string& port_name, void* data, size_t len)
	{
		if(cfg.isRTPorSharedBuffer(port_name) == 0) { //RTP
			abr::rtp_config rt_config = cfg.get_rtp_config(port_name);
			return (ERR_CODE)GraphApi->update(&rt_config, data, len);
		}
		else {
			abr::shared_buffer_config config = cfg.get_shared_buffer_config(port_name);
			return (ERR_CODE)GraphApi->update(&config, data, len);
		}
	}

	/**
	 * @brief Reads data from a specified port in the Remote Tile Processing (RTP) configuration.
	 * @note  Error checks, then reads data based on RTP Config name.
	 * @param		port_name		A pointer used to find RTP struct.
	 * @param		data			Location of read data.
	 * @param		len				Num of bytes of data being read.
	 */
	ERR_CODE read(const std::string& port_name, char* data, int len)
	{
		abr::rtp_config rt_config = cfg.get_rtp_config(port_name);
		return (ERR_CODE)GraphApi->read(&rt_config, data, len);
	}

	/**
	 * @brief Calls GraphApi wait function.
	 * @note  Waits for cores to finish executing. 
	 */
	void wait() {
		GraphApi->wait();
	}

	/**
	 * @brief 		Runs end function for GraphApi.
	 * @note  		Verifies that graph is properly configured.
	 * 		  		Waits for completion of graph functions. 
	 * 		  		Checks to see if all cores finished running. 
	 *        		Checks for errors and returns errors.
	 * 
	 * @return   	Error code depending on success/failure.
	 */
	ERR_CODE end() {
		return (ERR_CODE)GraphApi->end();
	}

	/**
	 * @brief		Calls end function for GMIO.
	 * @note		Checks for any buffers in queue, syncs them if needed.
	 */
	ERR_CODE gmio_wait(std::string gmio_name, bool syncmode) {
		auto gmioApiImpl = cfg.get_gmio_api_impl(gmio_name);
		return gmioApiImpl->wait(syncmode);
	}

	/**
	 * @brief Allocate memory for XAIe.
	 * 
	 * @param len   Size of allocated memory.
	 */
	void* malloc(uint32_t len){
		return xaiemem->malloc(len);
	}

	/**
	 * @brief Free allocated memory for XAIe.
	 * 
	 * @param addr   Location of memory to be freed. 
	 */
	void free(void* addr) {
		xaiemem->free(addr);
	}

	/**
	 * @brief	Processes error data and stores error payload.
	 * @return	Pointer to the collected error payloads. 
	 */
	void* BacktrackErrors()
	{
		XAie_ErrorMetaData MData = {};
#define PAYLOAD_SIZE (16)
		XAie_ErrorPayload tmp_payload[PAYLOAD_SIZE];
		MData.Payload = tmp_payload;
		MData.ArraySize = PAYLOAD_SIZE;
		MData.Cols.Start = DevInst.StartCol;
		MData.Cols.Num = DevInst.NumCols;
		std::vector<XAie_ErrorPayload> payload;

		do {
			XAie_BacktrackErrorInterrupts(&DevInst, &MData);
			for (u32 i = 0; i < MData.ErrorCount; i++) {
				XAie_ErrorPayload *p = &tmp_payload[i];
				payload.push_back(*p);
			}

		} while (MData.IsNextInfoValid);
		if (error_handling_cb) {
			error_handling_cb(payload, error_handling_cb_priv);
		}

		return static_cast<void *>(payload.data());
	}

	/**
	 * @brief	Calls profiling function on 1 port based on different options.
	 * @note	Options: 
	 * 			abr::_io_total_stream_running_to_idle_cycles for total 
	 * 			accumulated cycles in between stream running to stream idle events.
	 * 			abr::_io_stream_start_to_bytes_transferred_cycles for 
	 * 			Cycles in between the first event of stream running to the event that 
	 * 			the specified number of bytes are transferred (assuming the stream stops 
	 * 			right after the specified number of bytes are transferred).
	 * 			abr::_io_stream_start_difference_cycles for ycles elapsed between 
	 * 			the first stream running events of the two platform IO objects.
	 * 			abr::_io_stream_running_event_count for number of stream running events.
	 * @param	port_name	The port that this function will be called on.
	 * @param	option		The type of profiling to be run.
	 * @param	value		Specify additional information for profiling.
	 */
	int start_profiling(const std::string& port_name, int option, uint32_t value) {
		return EventApi->start_profiling(port_name, option, value);
	}

	/**
	 * @brief	Calls profiling function on 2 ports based on different options.
	 * @note	Options: 
	 * 			abr::_io_total_stream_running_to_idle_cycles for total 
	 * 			accumulated cycles in between stream running to stream idle events.
	 * 			abr::_io_stream_start_to_bytes_transferred_cycles for 
	 * 			cycles in between the first event of stream running to the event that 
	 * 			the specified number of bytes are transferred (assuming the stream stops 
	 * 			right after the specified number of bytes are transferred).
	 * 			abr::_io_stream_start_difference_cycles for cycles elapsed between 
	 * 			the first stream running events of the two platform IO objects.
	 * 			abr::_io_stream_running_event_count for number of stream running events.
	 * @param	port_name1	Name of port 1.
	 * @param	port_name2	Name of port 2. 
	 * @param	option		The type of profiling to be run.
	 * @param	value		Specify additional information for profiling.
	 */
	int start_profiling(const std::string& port_name1,
			const std::string& port_name2,
			int option,
			uint32_t value) {
		return EventApi->start_profiling(port_name1, port_name2, option,value);
	}
	
	/**
	 * @brief	Retrieve profiling data for a given event using a handle.
	 * @param	h	Handle used to find data.
	 */
	long long read_profiling(int h) {
		return EventApi->read_profiling(h);
	}

	/**
	 * @brief	Reset and cleanup performance monitoring and event-related resources.
	 * @param	h 	Handle used to find data. 
	 */
	void stop_profiling(int h) {
		return EventApi->stop_profiling(h);
	}
private:
	ConfigureParser cfg;
	XAie_DevInst DevInst={};
	std::shared_ptr<abr::graph_api> GraphApi;
	std::shared_ptr<abr::event> EventApi;
	///std::queue<TransactionInfo> me2gmSyncBDs;
	std::shared_ptr<XAieMem> xaiemem;
	void (*error_handling_cb)(std::vector<XAie_ErrorPayload> payloads, void *priv) = NULL;
	void *error_handling_cb_priv = NULL;
	cert_host::cert_api cert;

	int selftest_cert() {
		int ret;
		std::map<std::string_view, uint64_t> symbols;

		printf("test cert start.\n");
		ret = this->cert.load_cert(0);
		if (ret) {
			printf("load cert failed: %d\n", ret);
		}
		ret = this->cert.wakeup_cert(0);
		if (ret) {
			printf("Wakup cert col 0 failed\n");
		}
		cert.init_handshake();
		ret = cert.submit_test_command();

		if (ret) {
			printf("CERT: hsa test failed: %d\n", ret);
		} else {
			printf("CERT: hsa test PASSED.\n");
		}
		return ret;
	}

	int selftest_errorhandling() {
		int ret = 0;
		AieRC RC;
		std::vector<XAie_ErrorPayload> payloads;

		switch (DevInst.DevProp.DevGen) {
		case XAIE_DEV_GEN_AIE2PS:
		case XAIE_DEV_GEN_AIEML:
			XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
			break;
		case XAIE_DEV_GEN_AIE:
			XAie_UpdateNpiAddr(&DevInst, 0xF70A0000);
			break;
		default:
			printf("DevGen not found: %d\n", DevInst.DevProp.DevGen);
		}
		RC = XAie_PmRequestTiles(&DevInst, NULL, 0);
		if (RC == XAIE_OK) {
			printf("XAie_PmRequestTiles: OK\n");
		} else {
			printf("XAie_PmRequestTiles: %d\n", static_cast<int>(RC));
			return static_cast<int>(RC);
		}

		RC = XAie_ErrorHandlingInit(&DevInst);
		if (RC != XAIE_OK) {
			printf("Errorhandling init failed: %d\n", static_cast<int>(RC));
			return static_cast<int>(RC);
		}
		for (int i = DevInst.StartCol;
		     i < DevInst.StartCol + DevInst.NumCols;
		     i++) {
			generate_events_on_col(&DevInst, i);
		}

		printf("Finished Generating ErrorEvent\n");
		return ret;
	}
};

void* AIEGraph::malloc(size_t len) {
	return graph->malloc(len);
}

void AIEGraph::free(void* mem) {
	graph->free(mem);
}

/**
 * @brief	Creates a new Baremetal Graph object.
 * @param	GraphName		Name of graph. 
 */
AIEGraph::AIEGraph(std::string GraphName) {
	graph = std::make_shared<AIEGraphSim>(GraphName);

	if (!AIEGraphSim_List) {
		AIEGraphSim_List = new(std::vector<std::shared_ptr<AIEGraphSim>>)();
		AIEGraphSim_List->push_back(graph);
		abr_init_isr();
	} else {
		AIEGraphSim_List->push_back(graph);
	}
}

AIEGraph::~AIEGraph() {
}

void AIEGraph::selftestexit() {
	graph->selftestexit();
}

ERR_CODE AIEGraph::clock_enable() {
	return graph->clock_enable();
}

ERR_CODE AIEGraph::clock_disable() {
	return graph->clock_disable();
}

void AIEGraph::error_handling_cb_init(void *cb, void *priv) {
	graph->error_handling_cb_init(cb, priv);
}

void AIEGraph::init() {
	graph->init();
}

void AIEGraph::run(int num) {
	graph->run(num);
}

void AIEGraph::aie2gm_nb(char* name, char* data, int len) {
	graph->aie2gm_nb(name, data, len);
}

void AIEGraph::gm2aie_nb(char* name, char* data, int len) {
	graph->gm2aie_nb(name, data, len);
}
void AIEGraph::aie2gm(char* name, char* data, int len) {
	graph->aie2gm(name, data, len);
}

void AIEGraph::gm2aie(char* name, char* data, int len) {
	graph->gm2aie(name, data, len);
}

void AIEGraph::wait() {
	graph->wait();
}
ERR_CODE AIEGraph::end() {
	return (ERR_CODE)graph->end();
}

ERR_CODE AIEGraph::gmio_wait(std::string gmio_name,bool syncmode) {
	return graph->gmio_wait(gmio_name, syncmode);
}

ERR_CODE AIEGraph::update(const std::string& port_name, void* data, size_t len)
{
	return (ERR_CODE)graph->update(port_name, data, len);
}

void* AIEGraph::BacktrackErrors() {
	return graph->BacktrackErrors();
}

ERR_CODE AIEGraph::read(const std::string& port_name, char* data, int len)
{
	return (ERR_CODE)graph->read(port_name, data, len);
}

// External Buffer APIs
void AIEGraph::setAddress(char* externalbuffer_name, void* ptr){
	return graph->setAddress(externalbuffer_name, ptr);
}

ERR_CODE AIEGraph::wait(char* externalbuffer_name, std::string port_name){
	return graph->wait(externalbuffer_name, port_name);
}

ERR_CODE AIEGraph::gm2aie_nb(char* externalbuffer_name, std::string port_name){
	return graph->gm2aie_nb(externalbuffer_name, port_name);
}

ERR_CODE AIEGraph::aie2gm_nb(char* externalbuffer_name, std::string port_name){
	return graph->aie2gm_nb(externalbuffer_name, port_name);
}

ERR_CODE AIEGraph::setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2){
	return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
}
// End external Buffer API

int AIEGraph::start_profiling(const std::string& port_name, int option, uint32_t value) {
	return graph->start_profiling(port_name, option, value);
}
int AIEGraph::start_profiling(const std::string& port_name1,
		const std::string& port_name2,
		int option,
		uint32_t value) {
	return graph->start_profiling(port_name1, port_name2, option,value);
}
long long AIEGraph::read_profiling(int h) {
	return graph->read_profiling(h);
}
void AIEGraph::stop_profiling(int h) {
	graph->stop_profiling(h);
}
void abr_error_isr(void *data)
{
	(void)data;
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
	XScuGic_Disable(&xInterruptController, AIE_IRQ_VECT_ID_0);

	for (std::shared_ptr<AIEGraphSim> graph : *AIEGraphSim_List) {
		(void)graph->BacktrackErrors();
	}
	XScuGic_Enable(&xInterruptController, AIE_IRQ_VECT_ID_0);
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */
}

#ifdef __AIESIM__
/*
	 for simulator dll export
*/
class BaremetalGraphSimBaseImpl: public BaremetalGraphSimBase {
	public:
		BaremetalGraphSimBaseImpl(const char* gname) {
			std::string strGame(gname);
			graph = std::make_shared<AIEGraph>(strGame);
		}
		void selftestexit() override {
			graph->selftestexit();
		}
		ERR_CODE clock_enable() {
			return graph->clock_enable();
		}

		ERR_CODE clock_disable() {
			return graph->clock_disable();
		}

		void error_handling_cb_init(void *cb,
				    void *priv) override {
			graph->error_handling_cb_init(cb, priv);
		}
		void init() override {
			graph->init();
		}
		void run(int num) override {
			graph->run(num);
		}
		void aie2gm_nb(char* name, char* data, int len) override {
			graph->aie2gm_nb(name,data, len);
		}
		void gm2aie_nb(char* name, char* data, int len) override {
			graph->gm2aie_nb(name, data, len);
		}
		void aie2gm(char* name, char* data, int len) override {
			graph->aie2gm(name,data, len);
		}
		void gm2aie(char* name, char* data, int len) override {
			graph->gm2aie(name, data, len);
		}
		void wait() override {
			graph->wait();
		}

		// External Buffer APIs
		void setAddress(char* externalbuffer_name, void* ptr){
			return graph->setAddress(externalbuffer_name, ptr);
		}

		ERR_CODE wait(char* externalbuffer_name, std::string port_name){
			return graph->wait(externalbuffer_name, port_name);
		}

		ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name){
			return graph->gm2aie_nb(externalbuffer_name, port_name);
		}

		ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name){
			return graph->aie2gm_nb(externalbuffer_name, port_name);
		}

		ERR_CODE setupPingPongBuffers(char* externalbuffer_name, void* const ptr1,  void* const ptr2){
			return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
		}
		// End external Buffer API

		ERR_CODE end() {
			return graph->end();
		}
		ERR_CODE gmio_wait(std::string gmio_name,bool syncmode) override {
			return graph->gmio_wait(gmio_name,syncmode);
		}
		void* malloc(uint32_t len){
			return graph->malloc(len);
		}
		void free(void* addr) {
			graph->free(addr);
		}

		ERR_CODE update(const std::string& port_name, void* data, size_t len)
		{
			return graph->update(port_name, data, len);
		}

		ERR_CODE read(const std::string& port_name, char* data, int len)
		{
			return graph->read(port_name, data, len);
		}

		void* BacktrackErrors() {
			return graph->BacktrackErrors();
		}

		void gdbpoint() {
		}
  	int start_profiling(const std::string& port_name, int option, uint32_t value) {
			return graph->start_profiling(port_name, option, value);
		}
  	int start_profiling(const std::string& port_name1,
													 const std::string& port_name2,
													 int option,
													 uint32_t value) {
			return graph->start_profiling(port_name1, port_name2, option,value);
		}
  	long long read_profiling(int h) {
			return graph->read_profiling(h);
		}
  	void stop_profiling(int h) {
			return graph->stop_profiling(h);
		}
	private:
		std::shared_ptr<AIEGraph> graph;
};

extern "C" BaremetalGraphSimBase* createBaremetalGraphInstance(const char* gname) {
    return new BaremetalGraphSimBaseImpl(gname);
}
#endif
