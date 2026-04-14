/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_H__
#define __KT_APP_H__

#include "Kt_Lib.h"

#include "kt_app_cmds.h"

#include "os_funcs.h"
#include "os_threads.h"

#include "dal.h"
#include "dal_busses.h"
#include "dal_device_init.h"

/**.
 * TBD-DPN: Hard coded for now, will be made generic later.
 * Log and Database Location
 *
 * These directories are created by the Katana App Makefile.
 *
 */
#define KT_APP_LOG_DIR              "/home/lattice/Katana_1.0.0/kt_data/logs"
#define KT_APP_DB_DIR               "/home/lattice/Katana_1.0.0/kt_data/db"

/**
 * Logger File names (used in kt_app_logger.c)
 */
#define KT_APP_CMD_LOG_FILE_NAME    "kt_app_cmd.log"
#define KT_APP_STREAM_LOG_FILE_NAME "kt_app_stream.log"

/**
 * Face database file name (used in kt_app_face_db.c)
 */
#define KT_APP_FACE_DB_FILE_NAME    "kt_app_face_db.txt"

#define KT_APP_CMD_RESP_RETRY_COUNT 3

/* Size of the metadata buffer */
#define KT_APP_METADATA_BUFFER_SIZE                                            \
	(sizeof(struct PpeData) +                                                  \
	 KT_APP_MAX_FACES_IN_METADATA_STREAM * sizeof(struct FaceData))

/* Number of buffers in kt_app's queue / pool of buffers */
#define KT_APP_METADATA_BUFFER_POOL_SIZE 10

typedef struct metadata_buffer {
	uint8_t *p_buffer;
	uint32_t buff_len;
} metadata_buffer_t;

typedef struct metadata_buffer_queue {
	metadata_buffer_t buffers[KT_APP_METADATA_BUFFER_POOL_SIZE];
	uint32_t          head;
	uint32_t          tail;
	os_mutex_t        mutex;
} metadata_buffer_queue_t;

/**
 * This is the memory space for exchanging information between kt_app_main and
 * kt_app_interactive. Since multiple entities can update the contents,
 * we use a mutex to protect the access.
 */
struct kt_app_shmem {
	/* Mutex for accessing the shared memory */
	os_mutex_t mutex;

	/* Command from kt_app_interactive thread to kt_app_main thread */
	struct kt_app_request request;

	/* Response from kt_app_main thread to kt_app_interactive thread */
	struct kt_app_response response;

	/* Flag to indicate if the command is pending */
	bool cmd_pending;
};

struct kt_app_ctx {
	/* Holding the KtHandle for now */
	Kt_Handle kt_handle;

	/* Log files */
	/* Command log file pointer */
	FILE *cmd_log_file;
	/* Command log file path */
	char cmd_log_file_path[PATH_MAX];
	/* Stream log file pointer */
	FILE *stream_log_file;
	/* Stream log file path */
	char stream_log_file_path[PATH_MAX];

	/* Face database */
	FILE *face_db_file;
	/* Face database file path */
	char face_db_file_path[PATH_MAX];

	/* Flag to signal if the application should exit (volatile: IPC / idle poll
	 * it) */
	volatile bool should_exit;

	/* A pointer to the shared memory */
	struct kt_app_shmem *kt_app_shmem;

	/* A queue of metadata buffers */
	metadata_buffer_queue_t buffer_queue;

	/* In-memory buffer for face profiles */
	struct kt_app_face_profile face_profiles[KT_APP_MAX_FACE_PROFILES_ON_DISK];
	/* Number of face profiles in the in-memory buffer */
	uint32_t face_profile_count;

	/**
	 * IPC related variables
	 */
	/* Socket FDs for request-response communication */
	int req_resp_server_sockfd;
	int req_resp_client_sockfd;
	/* Socket path for request-response communication */
	char req_resp_socket_path[PATH_MAX];
	/* Socket FDs for stream communication */
	int stream_server_sockfd;
	int stream_client_sockfd;
	/* Socket path for stream communication */
	char stream_socket_path[PATH_MAX];

	/* Flag to indicate if the request-response IPC is connected */
	bool req_resp_ipc_connected;
	/* Flag to indicate if the stream IPC is connected */
	bool stream_ipc_connected;
};

void kt_app_idle_cb(App_Handle app_handle);

void kt_app_generate_face_id_done_cb(App_Handle app_handle,
									 uint32_t   face_id,
									 Kt_RetCode op_status);

void kt_app_face_identified_cb(App_Handle      app_handle,
							   struct PpeData *p_ppe_data);

void kt_app_face_not_identified_cb(App_Handle      app_handle,
								   struct PpeData *p_ppe_data);

void kt_app_metadata_buffer_queue_init(App_Handle app_handle);

void kt_app_metadata_buffer_queue_fini(App_Handle app_handle);

void kt_app_metadata_buffer_queue_add(App_Handle app_handle,
									  uint8_t   *p_buffer,
									  uint32_t   buff_len);

void kt_app_metadata_buffer_queue_get(App_Handle app_handle,
									  uint8_t  **p_buffer,
									  uint32_t  *buff_len);

/**
 * kt_app_interactive is the main function for the kt_app_interactive thread.
 * It is responsible for interacting with the user and responding to commands.
 */
void *kt_app_interactive(void *arg);

/**
 * kt_app_main is the main function for the kt_app_main thread.
 * It is responsible for initializing the kt_app and starting the dispatcher.
 */
void *kt_app_main(void *arg);

/**
 * kt_app_ipc is the function for the kt_app_ipc thread.
 * It is responsible for handling the IPC communication with the kt_app_main
 * thread.
 */
void *kt_app_ipc(void *arg);

#endif