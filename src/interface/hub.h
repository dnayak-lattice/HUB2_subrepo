/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __HUB_H__
#define __HUB_H__

#include "types.h"

#include "os_funcs.h"

/* PATH_MAX definition for platform compatibility */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* Maximum number of DAL handles supported by HUB. */
#define DAL_MAX_COUNT 10

/* NULL definition for platform compatibility */
#ifndef NULL
#define NULL (void *)0
#endif

/**
	HUB return codes
*/
enum hub_ret_code {
	HUB_SUCCESS        = 0,
	/* HUB error codes */
	HUB_FAILURE_MARKER = -256,
	HUB_FAILURE_PREINIT,
	HUB_FAILURE_INIT,
	HUB_FAILURE_GARD_DISCOVER,
	HUB_FAILURE_FINI,
	HUB_FAILURE_GARD_CONFIG,
	HUB_FAILURE_WRITE_REG,
	HUB_FAILURE_READ_REG,
	HUB_FAILURE_SEND_DATA,
	HUB_FAILURE_RECV_DATA,
	HUB_FAILURE_SEND_PAUSE_PIPELINE,
	HUB_FAILURE_SEND_RESUME_PIPELINE,
};

/* HUB handle / context states */
enum hub_ctx_state {
	HUB_IN_ERROR = -1,
	HUB_BEGIN    = 0,
	HUB_PREINIT_DONE,
	HUB_DISCOVER_DONE,
	HUB_INIT_DONE
};

#define hub_pr_info(fmt, args...)                                              \
	os_fprintf(stdout, "HUB_INFO:%s:%d:%s(): " fmt, __FILE__, __LINE__,        \
			   __func__, ##args);

#if defined(HUB_DEBUG)
#define hub_pr_dbg(fmt, args...)                                               \
	os_fprintf(stdout, "HUB_DBG:%s:%d:%s(): " fmt, __FILE__, __LINE__,         \
			   __func__, ##args);
#else
#define hub_pr_dbg(fmt, args...) ((void)0)
#endif

#define hub_pr_err(fmt, args...)                                               \
	os_fprintf(stderr, "HUB_ERR:%s:%d:%s(): " fmt, __FILE__, __LINE__,         \
			   __func__, ##args);

#define hub_pr_warn(fmt, args...)                                              \
	os_fprintf(stdout, "HUB_WARN:%s:%d:%s(): " fmt, __FILE__, __LINE__,        \
			   __func__, ##args);

#define hub_pr_prod(fmt, args...)                                              \
	os_fprintf(stdout, "HUB_PROD:%s:%d:%s(): " fmt, __FILE__, __LINE__,        \
			   __func__, ##args);

/* Opaque handle to a HUB instance, returned from the hub_init() call */
typedef void *hub_handle_t;

/* Opaque handle for each GARD listed in the hub handle */
typedef void *gard_handle_t;

/**
 * Opaque handle for the Device Adaptation Layer.
 * Forward declaration, actual definition in dal.h
 *
 * This handle is an input argument to any operations performed on a device.
 */
typedef void *dal_handle_t;

/**
 * Bus listing for HUB <-> GARD communications
 * This will be used to create:
 * 1. An enum for the bus type
 * 2. An array of strings where the string matches the enum name
 */
#define FOR_EACH_BUS(BUS)                                                      \
	BUS(HUB_GARD_BUS_UNKNOWN)                                                  \
	BUS(HUB_GARD_BUS_I2C)                                                      \
	BUS(HUB_GARD_BUS_UART)                                                     \
	BUS(HUB_GARD_BUS_PCIE)                                                     \
	BUS(HUB_GARD_BUS_USB)                                                      \
	BUS(HUB_GARD_BUS_MIPI_CSI2)                                                \
	BUS(HUB_GARD_NR_BUSSES)

#define GEN_BUS_ENUM(BUS)      BUS,
#define GEN_BUS_STRING(STRING) #STRING,

/* Enum for all HUB <-> GARD busses */
enum hub_gard_bus_types {
	FOR_EACH_BUS(GEN_BUS_ENUM)
};

/**
 * Bus policy listing for HUB <-> GARD communications
 * This will be used to create:
 * 1. An enum for the bus policy
 * 2. An array of strings where the string matches the enum name
 */
#define FOR_EACH_BUS_POLICY(POLICY)                                            \
	POLICY(HUB_GARD_BUS_POLICY_UNKNOWN)                                        \
	POLICY(HUB_GARD_BUS_POLICY_USE_I2C)                                        \
	POLICY(HUB_GARD_BUS_POLICY_USE_UART)                                       \
	POLICY(HUB_GARD_BUS_POLICY_USE_PCIE)                                       \
	POLICY(HUB_GARD_BUS_POLICY_USE_USB)                                        \
	POLICY(HUB_GARD_BUS_POLICY_USE_FASTEST_BUS)                                \
	POLICY(HUB_GARD_BUS_NR_POLICY_TYPES)

#define GEN_BUS_POLICY_ENUM(POLICY)   POLICY,
#define GEN_BUS_POLICY_STRING(STRING) #STRING,

/* Enum for all HUB <-> GARD bus policies */
enum hub_gard_bus_policy {
	FOR_EACH_BUS_POLICY(GEN_BUS_POLICY_ENUM)
};

/**
 * HUB configuration structure
 *
 * This is to be filled in by the application before calling hub_preinit().
 *
 * Details of the input parameters for the hub_config structure:
 * - num_busses: number of busses the HUB can use
 * - dal_busses: array of DAL busses the HUB can use
 */
struct hub_config {
	uint32_t     num_busses;
	dal_handle_t dal_busses[DAL_MAX_COUNT];
};

/**
 * This is the back-end of the GARD handle.
 * Used internally for HUB operations related to a GARD.
 *
 * Details of the input parameters for the hub_gard_info structure:
 * - hub: the handle to the HUB
 * - gard_profile_id: the profile ID of the GARD
 * - gard_name: the name of the GARD
 * - bus_policy: the bus policy of the GARD
 * - active_dal_bus: the active DAL bus of the GARD
 * - num_busses: the number of busses of the GARD
 * - p_dal_busses: the pointer to the DAL busses of the GARD
 * - default_originator: whether the GARD is the default originator
 * - command_execution: whether the GARD is executing commands
 * - p_app_layer_cbs: the pointer to the App Layer callbacks
 */
struct hub_gard_info {
	hub_handle_t                    hub;
	uint32_t                        gard_profile_id;
	char                            gard_name[PATH_MAX];
	enum hub_gard_bus_policy        bus_policy;
	dal_handle_t                    active_dal_bus;
	uint32_t                        num_busses;
	dal_handle_t                   *p_dal_busses;
	bool                            default_originator;
	bool                            command_execution;
	struct hub_app_layer_callbacks *p_app_layer_cbs;
};

/**
 * This is the back-end of the HUB handle.
 * Used internally for HUB operations.
 *
 * Details of the input parameters for the hub_ctx structure:
 * - hub_state: state of the HUB
 * - p_config: pointer to the HUB configuration
 * - num_gards: number of GARDS discovered on the HUB
 * - p_gards: pointer to the array of GARDS discovered on the HUB
 */
struct hub_ctx {
	enum hub_ctx_state    hub_state;
	struct hub_config    *p_config;
	uint32_t              num_gards;
	struct hub_gard_info *p_gards;
};

/*********************************************************/
/*********************************************************/
/* PUBLIC API FUNCTIONS */
/*********************************************************/
/*********************************************************/
/**
 * Get the version string of the HUB
 *
 * @return A pointer to the version string.
 */
const char *hub_get_version_string(void);

/**
 * Pre-initialize the HUB.
 *
 * This is typically the first call made by the application to the HUB.
 *
 * The App is expected to fill in broad details of bus/communication interfaces
 * such as bus number, bus device node, etc. HUB then fills the details of the
 * bus interfaces, such as slave address for I2C, baud rate for UART, etc. On
 * success, HUB returns a handle to the HUB instance. On failure, HUB returns
 * NULL.
 *
 * @param p_config Pointer to the HUB configuration structure.
 * @return A pointer to the HUB handle.
 *         NULL if the HUB is not pre-initialized successfully.
 */
hub_handle_t hub_preinit(struct hub_config *p_config);

/**
 * Discover GARDS on the HUB.
 *
 * This function is used to discover GARDS on the HUB and populate the GARD
 * information in the HUB context. HUB sends discovery commands to the GARDs on
 * the bus interfaces, and instantiates GARD handles for each GARD. On success,
 * HUB returns HUB_SUCCESS. On failure, HUB returns HUB_FAILURE_GARD_DISCOVER.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the GARDS are discovered successfully,
 *         HUB_FAILURE_GARD_DISCOVER if the GARDS are not discovered.
 */
enum hub_ret_code hub_discover_gards(hub_handle_t hub_handle);

/**
 * Initialize the HUB
 *
 * Here, HUB initializes the instance to the state where it can respond to and
 * execute operational commands from the App.
 *
 * In the present version, where HUB is connected to a GARD, the
 * hub_discover_gards() before this call. Currently, this function assigns the
 * GARD's active bus as per the bus policy.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the HUB is initialized successfully,
 *         HUB_FAILURE_INIT if the HUB is not initialized successfully.
 */
enum hub_ret_code hub_init(hub_handle_t hub_handle);

/**
 * Finalize the HUB.
 *
 * This function is used to finalize the HUB and free the resources.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the HUB is finalized successfully,
 *         HUB_FAILURE_FINI if the HUB is not finalized successfully.
 */
enum hub_ret_code hub_fini(hub_handle_t hub_handle);

/**
 * Get the number of GARDS on the HUB
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return The number of GARDS on the HUB.
 */
uint32_t hub_get_num_gards(hub_handle_t hub_handle);

/**
 * Get the GARD handle for a given index
 *
 * @param hub_handle The handle to the HUB.
 * @param gard_index The index of the GARD.
 *
 * @return The GARD handle.
 *         NULL if the GARD handle is not found.
 */
gard_handle_t hub_get_gard_handle(hub_handle_t hub_handle, uint32_t gard_index);

/**
 * Check if GARD sent a GARD (or App Layer) command and execute it. This could
 * either be a command or data transfer.
 *
 * @param p_gard_handle The GARD handle.
 *
 * @return True if a GARD command was found and executed successfully, false
 * otherwise.
 */
bool hub_check_and_execute_gard_command(gard_handle_t p_gard_handle);

/******************************************************************************
 * HUB <-> GARD data movement APIs
 ******************************************************************************/
/**
 * Write a given 32-bit value to a register address inside the GARD memory map
 * represented by the GARD handle.
 *
 * @param p_gard_handle The GARD handle.
 * @param reg_addr The register address inside the GARD memory map.
 * @param value The 32-bit value to write to the register.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_WRITE_REG on failure
 */
enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
									 uint32_t       reg_addr,
									 const uint32_t value);

/**
 * Read the 32-bit value of a register address inside the GARD memory map
 * represented by the GARD handle.
 *
 * @param p_gard_handle The GARD handle.
 * @param reg_addr The register address inside the GARD memory map.
 * @param p_value The pointer to a 32-bit value to read from the register.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_READ_REG on failure
 */
enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
									uint32_t      reg_addr,
									uint32_t     *p_value);

/**
 * Send a data buffer of a specified size from HUB to an
 * address in the GARD memory map represented by the GARD handle.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The pointer to the data buffer to send to the GARD.
 * @param addr The address in the GARD memory map to send the data to.
 * @param count The number of bytes to send.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_SEND_DATA on failure
 */
enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
										const void   *p_buffer,
										uint32_t      addr,
										uint32_t      count);

/**
 * Receive data of specified size from an address in the
 * GARD memory map represented	by the GARD handle, into a HUB
 * data buffer.
 *
 * Note: If this function is called to receive image data from GARD,
 * say, after hub_capture_rescaled_image_from_gard() is called, then
 * HUB / Host Applicaiton should also call hub_send_resume_pipeline() after
 * receiving the image content to signal GARD FW to resume the paused AI
 * workload.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The pointer to the data buffer to receive the data into.
 * @param addr The address in the GARD memory map to receive the data from.
 * @param count The number of bytes to receive.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_RECV_DATA on failure
 */
enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
										  void         *p_buffer,
										  uint32_t      addr,
										  uint32_t      count);

/**
 * Send a pause pipeline command to the GARD
 *
 * @param p_gard_handle The GARD handle.
 * @param camera_id The camera ID to pause the pipeline for.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_PAUSE_PIPELINE on failure
 */
enum hub_ret_code hub_send_pause_pipeline_cmd(gard_handle_t p_gard_handle,
											  uint8_t       camera_id);

/**
 * Send a resume pipeline command to the GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param camera_id The camera ID to resume the pipeline for.
 *
 * @return HUB_SUCCESS on success, HUB_FAILURE_RESUME_PIPELINE on failure
 */
enum hub_ret_code hub_send_resume_pipeline_cmd(gard_handle_t p_gard_handle,
											   uint8_t       camera_id);

/*******************************************************************************
 * The following functions are used by the App Layer to send (and receive)
 * commands, data and responses from the App Layer that is running over GARD.
 * HUB only provides a channel to route these App Layer commands and data to the
 * App Layer running above GARD.
 ******************************************************************************/

/**
 * Execute an App Layer request
 *
 * @param p_gard_handle The GARD handle.
 * @param app_request_buffer The pointer to the App Layer request buffer.
 * @param app_request_buffer_size The size of the App Layer request buffer.
 * @param app_response_buffer The pointer to the App Layer response buffer.
 * @param app_response_buffer_size The size of the App Layer response buffer.
 *
 * @return True if the request is executed successfully, false otherwise.
 */
bool hub_app_layer_request(gard_handle_t p_gard_handle,
						   void         *app_request_buffer,
						   uint32_t      app_request_buffer_size,
						   void         *app_response_buffer,
						   uint32_t      app_response_buffer_size);

/*******************************************************************************
 * The following functions are provided by the App Layer and called by HUB.
 * The App Layer is expected to provide actual implementations of these
 * functions.
 ******************************************************************************/

/**
 * The size of the peek buffer is enough for the App Layer to determine the type
 * of data that is arriving from App Layer running over GARD.
 */
#define APP_LAYER_PEEK_BUFFER_SIZE 4

/**
 * Request App Layer to get a buffer for data that is arriving from App Layer
 * running over GARD. App Layer is sent a peek buffer which helps it decide what
 * buffer to use for receiving the data. The size of this peek buffer is defined
 * by the macro APP_LAYER_PEEK_BUFFER_SIZE.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The pointer to the pointer to the App Layer buffer to get.
 * @param p_buffer_size The pointer to the size of the App Layer buffer.
 * @param p_peek_buffer The pointer to the pointer to the Peek buffer to get.
 * @param p_peek_buffer_size The pointer to the size of the Peek buffer.
 *
 * @return True if the buffer is successfully got, false otherwise.
 */
typedef bool (*app_layer_get_buffer_for_gard_send_data)(
	gard_handle_t p_gard_handle,
	void         *app_handle,
	void        **p_buffer,
	uint32_t     *p_buffer_size,
	void         *p_peek_buffer,
	uint32_t      peek_buffer_size);

/**
 * App Layer handles the data that was sent by the App Layer running over GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The pointer to the data buffer that contains the data sent by
 *                 App Layer running over GARD.
 * @param buffer_size The size of the data buffer that contains the data sent by
 *                    App Layer running over GARD.
 *
 * @return True if the data is successfully sent, false otherwise
 */
typedef bool (*app_layer_handle_gard_send)(gard_handle_t p_gard_handle,
										   void         *app_handle,
										   void         *p_buffer,
										   uint32_t      buffer_size);

/**
 * App Layer handles the request that was sent by the App Layer running over
 * GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_request The pointer to the request buffer to send.
 * @param request_size The size of the request buffer to send.
 * @param p_response The pointer to the response buffer to receive.
 * @param response_size The size of the response buffer to receive.
 *
 * @return True if the request is successfully handled, false otherwise
 */
typedef bool (*app_layer_handle_gard_request)(gard_handle_t p_gard_handle,
											  void         *app_handle,
											  void         *p_request,
											  uint32_t      request_size,
											  void         *p_response,
											  uint32_t      response_size);

/**
 * hub_app_layer_callbacks is a structure that contains the callbacks provided
 * by the App Layer.
 * - app_handle is a pointer to the App Layer handle.
 * - get_buffer_for_gard_send_data is a callback function that is called by HUB
 *	 when it needs to get a buffer for sending data to GARD.
 * - handle_gard_send is a callback function that is called by HUB when it needs
 *	 to handle data that was sent by GARD.
 * - handle_gard_request is a callback function that is called by HUB when it
 * 	 needs to handle a request that was sent by GARD.
 */
struct hub_app_layer_callbacks {
	void                                   *app_handle;
	app_layer_get_buffer_for_gard_send_data get_buffer_for_gard_send_data;
	app_layer_handle_gard_send              handle_gard_send;
	app_layer_handle_gard_request           handle_gard_request;
};

/**
 * Set the App Layer callbacks.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_callbacks The pointer to the App Layer callbacks.
 *
 * @return True if the App Layer callbacks are set successfully, false
 * otherwise.
 */
bool hub_set_app_layer_callbacks(gard_handle_t                   p_gard_handle,
								 struct hub_app_layer_callbacks *p_callbacks);

#endif /* __HUB_H__ */
