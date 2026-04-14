/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_IPC_H__
#define __KT_APP_IPC_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include "kt_app.h"
#include "kt_app_cmds.h"
#include "kt_app_logger.h"
#include "kt_app_req_resp.h"

/* Kt App IPC Functions */

/**
 * Wake IPC loop periodically to observe should_exit (do not rely on close()
 * unblocking accept).
 */
#define KT_APP_IPC_POLL_TIMEOUT_MS 250

/**
 * Close the stream listen socket so a blocked accept() returns (call before
 * join).
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_ipc_stop_listen(App_Handle app_handle);

/**
 * Create a socket
 *
 * @param p_server_sockfd The server socket file descriptor
 * @param socket_path The path to the socket file
 *
 * @return int 0 on success, -1 on failure
 */
int  kt_app_ipc_create_socket(int *p_server_sockfd, const char *socket_path);

/**
 * Single preferred API: robust send on a specific socket descriptor
 *
 * @param sockfd The socket file descriptor
 * @param data The data to send
 * @param size The size of the data
 *
 * @return int 0 on success, -1 on failure
 */
int  kt_app_ipc_send_data_on_fd(int sockfd, const void *data, size_t size);

/**
 * Receive up to size bytes once (thin wrapper around recv)
 *
 * @param sockfd The socket file descriptor
 * @param buf The buffer to receive the data into
 * @param size The size of the data
 *
 * @return ssize_t The number of bytes received, -1 on failure
 */
ssize_t kt_app_ipc_recv_data_on_fd(int sockfd, void *buf, size_t size);

#endif /* __KT_APP_IPC_H__ */