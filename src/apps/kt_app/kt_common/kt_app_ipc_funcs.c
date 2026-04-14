/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app_ipc_funcs.h"

/* Kt App IPC Functions */

/**
 * Helper function to create a socket
 *
 * @param p_server_sockfd The server socket file descriptor
 * @param socket_path The path to the socket file
 *
 * @return int 0 on success, -1 on failure
 */
int kt_app_ipc_create_socket(int *p_server_sockfd, const char *socket_path)
{
	*p_server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*p_server_sockfd < 0) {
		os_pr_err("Failed to create socket\n");
		return -1;
	}

	/* Remove the socket file if it exists */
	if (access(socket_path, F_OK) == 0) {
		if (unlink(socket_path) < 0) {
			os_pr_err("Failed to remove socket file\n");
			close(*p_server_sockfd);
			return -1;
		}
	}

	/* Create a socket address */
	struct sockaddr_un addr = {0};
	addr.sun_family         = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
	/* Bind the socket to the path */
	if (bind(*p_server_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		os_pr_err("Failed to bind socket\n");
		close(*p_server_sockfd);
		return -1;
	}

	/* Listen for connections */
	if (listen(*p_server_sockfd, 10) < 0) {
		os_pr_err("Failed to listen for connections\n");
		close(*p_server_sockfd);
		return -1;
	}

	os_pr_info("Socket created and listening for connections\n");

	return 0;
}

/**
 * Stop listening for connections on the socket
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_ipc_stop_listen(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	if (p_app_ctx->stream_server_sockfd >= 0) {
		(void)close(p_app_ctx->stream_server_sockfd);
		p_app_ctx->stream_server_sockfd = -1;
	}
}

/**
 * Send data on a socket
 *
 * @param sockfd The socket file descriptor
 * @param data The data to send
 * @param size The size of the data
 *
 * @return int 0 on success, -1 on failure
 */
int kt_app_ipc_send_data_on_fd(int sockfd, const void *data, size_t size)
{
	if (sockfd < 0 || data == NULL || size == 0) {
		errno = EINVAL;
		return -1;
	}
	size_t off = 0;
	while (off < size) {
		ssize_t w = send(sockfd, (const uint8_t *)data + off, size - off,
						 MSG_NOSIGNAL);
		if (w < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}
		off += (size_t)w;
	}
	return 0;
}

/**
 * Receive data on a socket
 *
 * @param sockfd The socket file descriptor
 * @param buf The buffer to receive the data into
 * @param size The size of the data
 *
 * @return ssize_t The number of bytes received, -1 on failure
 */
ssize_t kt_app_ipc_recv_data_on_fd(int sockfd, void *buf, size_t size)
{
	if (sockfd < 0 || buf == NULL || size == 0) {
		errno = EINVAL;
		return -1;
	}
	ssize_t r;
	do {
		r = recv(sockfd, buf, size, 0);
	} while (r < 0 && errno == EINTR);
	return r;
}
