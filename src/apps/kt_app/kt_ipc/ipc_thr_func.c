/* *****************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app.h"
#include "kt_app_logger.h"
#include "kt_app_ipc_funcs.h"

/* Kt App IPC Thread Functions */

/* --- Internal helpers for req/resp path --- */
/**
 * Receive all data from a socket
 *
 * @param fd The socket file descriptor
 * @param buf The buffer to receive the data into
 * @param len The length of the data to receive
 *
 * @return int 0 on success, -1 on failure
 */
static int recv_all(int fd, void *buf, size_t len)
{
	size_t off = 0;
	while (off < len) {
		ssize_t r = recv(fd, (uint8_t *)buf + off, len - off, 0);
		if (r == 0) {
			return -1; /* EOF */
		}
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}
		off += (size_t)r;
	}
	return 0;
}

/**
 * Send all data to a socket
 *
 * @param fd The socket file descriptor
 * @param buf The data to send
 * @param len The length of the data to send
 *
 * @return int 0 on success, -1 on failure
 */
static int send_all(int fd, const void *buf, size_t len)
{
	size_t off = 0;
	while (off < len) {
		ssize_t w = send(fd, (const uint8_t *)buf + off, len - off, MSG_NOSIGNAL);
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
 * Wait for a response from the kt_app_main thread
 *
 * @param p_app_ctx The app context
 *
 * @return void
 */
static void wait_for_response(struct kt_app_ctx *p_app_ctx)
{
	for (;;) {
		bool pending;
		os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
		pending = p_app_ctx->kt_app_shmem->cmd_pending;
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
		if (!pending || p_app_ctx->should_exit) {
			break;
		}
		os_sleep_ms(10);
	}
}

/**
 * Send a header only response to a socket
 *
 * @param fd The socket file descriptor
 * @param resp The response to send
 *
 * @return int 0 on success, -1 on failure
 */
static int send_header_only(int fd, const struct kt_app_response *resp)
{
	struct {
		uint32_t cmd;
		int32_t  ret_code;
	} header = {
		.cmd      = (uint32_t)resp->cmd,
		.ret_code = (int32_t)resp->ret_code,
	};
	return send_all(fd, &header, sizeof(header));
}

/**
 * Send a list of profiles payload to a socket
 *
 * @param fd The socket file descriptor
 * @param resp The response to send
 *
 * @return int 0 on success, -1 on failure
 */
static int send_list_profiles_payload(int fd, const struct kt_app_response *resp)
{
	uint32_t face_count = resp->list_face_profiles_response.face_count;
	if (send_all(fd, &face_count, sizeof(face_count)) != 0) {
		return -1;
	}
	for (uint32_t i = 0; i < face_count; i++) {
		const struct kt_app_face_profile *p =
			&resp->list_face_profiles_response.face_profiles[i];
		if (send_all(fd, &p->face_id, sizeof(p->face_id)) != 0) {
			return -1;
		}
		if (send_all(fd, p->face_name, PATH_MAX) != 0) {
			return -1;
		}
	}
	return 0;
}

/**
 * Read an update profile payload from a socket
 *
 * @param fd The socket file descriptor
 * @param req The request to read
 *
 * @return int 0 on success, -1 on failure
 */
static int read_update_profile_payload(int fd, struct kt_app_request *req)
{
	uint32_t face_id = 0;
	char     face_name[KT_APP_MAX_FACE_NAME_LENGTH] = {0};

	if (recv_all(fd, &face_id, sizeof(face_id)) != 0) {
		return -1;
	}
	if (recv_all(fd, face_name, sizeof(face_name)) != 0) {
		return -1;
	}
	req->update_face_profile_request.face_id = face_id;
	os_memcpy(req->update_face_profile_request.face_name, face_name,
			  sizeof(req->update_face_profile_request.face_name));
	/* Ensure NUL termination */
	if (sizeof(req->update_face_profile_request.face_name) > 0) {
		req->update_face_profile_request
			.face_name[sizeof(req->update_face_profile_request.face_name) - 1] =
			'\0';
	}
	return 0;
}

/**
 * Handle a request from a connected req/resp client
 *
 * @param p_app_ctx The app context
 *
 * @return void
 */
static void handle_req_resp_command(struct kt_app_ctx *p_app_ctx)
{
	/* Read 4-byte cmd */
	uint32_t cmd_u32 = 0;
	if (recv_all(p_app_ctx->req_resp_client_sockfd, &cmd_u32, sizeof(cmd_u32)) !=
		0) {
		os_pr_err("recv(cmd) failed\n");
		return;
	}

	struct kt_app_request request = (struct kt_app_request){0};
	request.cmd                   = (enum kt_app_cmd)cmd_u32;

	/* Per-command request payload (if any) */
	switch (request.cmd) {
	case KT_APP_CMD_UPDATE_FACE_PROFILE:
		if (read_update_profile_payload(p_app_ctx->req_resp_client_sockfd,
										&request) != 0) {
			os_pr_err("Failed to read UpdateFaceProfile payload\n");
			return;
		}
		break;
	default:
		/* No extra payload for minimal requests */
		break;
	}

	/* Dispatch to main thread and wait for response */
	kt_app_send_request((App_Handle)p_app_ctx, &request);

	/* For async commands, acknowledge immediately and let callback notify completion */
	if (request.cmd == KT_APP_CMD_GENERATE_FACE_ID_ASYNC) {
		struct {
			uint32_t cmd;
			int32_t  ret_code;
		} header = {
			.cmd = (uint32_t)KT_APP_CMD_GENERATE_FACE_ID_ASYNC,
			.ret_code = (int32_t)KT_APP_RET_CODE_PENDING,
		};
		(void)send_all(p_app_ctx->req_resp_client_sockfd, &header, sizeof(header));
		return;
	}

	wait_for_response(p_app_ctx);

	struct kt_app_response response = {0};
	kt_app_get_response((App_Handle)p_app_ctx, &response);

	/* Always send header first */
	if (send_header_only(p_app_ctx->req_resp_client_sockfd, &response) != 0) {
		os_pr_err("send(header) failed\n");
		return;
	}

	/* Optional payloads based on command and status */
	switch (response.cmd) {
	case KT_APP_CMD_LIST_FACE_PROFILES:
		if (response.ret_code == KT_APP_SUCCESS) {
			(void)send_list_profiles_payload(p_app_ctx->req_resp_client_sockfd,
											 &response);
		}
		break;
	default:
		/* No extra payload */
		break;
	}
}

/**
 * IPC thread function
 *
 * @param arg The argument
 *
 * @return void *
 */
void *kt_app_ipc(void *arg)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)arg;
	int                ret       = 0;

	/* Create the stream server socket */
	ret = kt_app_ipc_create_socket(&p_app_ctx->stream_server_sockfd,
								   p_app_ctx->stream_socket_path);
	if (ret < 0) {
		os_pr_err("Failed to create stream server socket\n");
		return NULL;
	}
	p_app_ctx->stream_client_sockfd = -1;

	/* Create the request-response server socket */
	ret = kt_app_ipc_create_socket(&p_app_ctx->req_resp_server_sockfd,
								   p_app_ctx->req_resp_socket_path);
	if (ret < 0) {
		os_pr_err("Failed to create request-response server socket\n");
		return NULL;
	}
	p_app_ctx->req_resp_client_sockfd = -1;

	while (!p_app_ctx->should_exit) {
		struct pollfd pfds[3];
		nfds_t        nfds = 0;

		/* Stream listen socket */
		pfds[nfds].fd     = p_app_ctx->stream_server_sockfd;
		pfds[nfds].events = POLLIN;
		nfds++;

		/* Req/Resp listen socket */
		pfds[nfds].fd     = p_app_ctx->req_resp_server_sockfd;
		pfds[nfds].events = POLLIN;
		nfds++;

		/* Connected Req/Resp client socket (if any) */
		if (p_app_ctx->req_resp_client_sockfd >= 0) {
			pfds[nfds].fd     = p_app_ctx->req_resp_client_sockfd;
			pfds[nfds].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
			nfds++;
		}

		int pr = poll(pfds, nfds, KT_APP_IPC_POLL_TIMEOUT_MS);
		if (pr < 0) {
			if (errno == EINTR) {
				continue;
			}
			os_pr_err("poll: errno %d\n", errno);
			break;
		}
		if (p_app_ctx->should_exit) {
			break;
		}
		if (pr == 0) {
			continue;
		}

		nfds_t idx = 0;

		/* New stream client? */
		if (pfds[idx].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (pfds[idx].revents & POLLIN) {
			int cfd = accept(p_app_ctx->stream_server_sockfd, NULL, NULL);
			if (cfd >= 0) {
				if (p_app_ctx->stream_client_sockfd >= 0) {
					(void)shutdown(p_app_ctx->stream_client_sockfd, SHUT_RDWR);
					(void)close(p_app_ctx->stream_client_sockfd);
				}
				p_app_ctx->stream_client_sockfd = cfd;
				p_app_ctx->stream_ipc_connected = true;
				os_pr_info("Stream client connected\n");
			} else if (errno != EINTR) {
				os_pr_err("accept(stream): errno %d\n", errno);
			}
		}
		idx++;

		/* New req/resp client? */
		if (pfds[idx].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (pfds[idx].revents & POLLIN) {
			int cfd = accept(p_app_ctx->req_resp_server_sockfd, NULL, NULL);
			if (cfd >= 0) {
				if (p_app_ctx->req_resp_client_sockfd >= 0) {
					(void)shutdown(p_app_ctx->req_resp_client_sockfd, SHUT_RDWR);
					(void)close(p_app_ctx->req_resp_client_sockfd);
				}
				p_app_ctx->req_resp_client_sockfd = cfd;
				p_app_ctx->req_resp_ipc_connected = true;
				os_pr_info("Req/Resp client connected\n");
			} else if (errno != EINTR) {
				os_pr_err("accept(req_resp): errno %d\n", errno);
			}
		}
		idx++;

		/* Handle a request from connected req/resp client */
		if (p_app_ctx->req_resp_client_sockfd >= 0 && idx < nfds) {
			short ev = pfds[idx].revents;
			if (ev & (POLLHUP | POLLERR | POLLNVAL)) {
				(void)shutdown(p_app_ctx->req_resp_client_sockfd, SHUT_RDWR);
				(void)close(p_app_ctx->req_resp_client_sockfd);
				p_app_ctx->req_resp_client_sockfd = -1;
				p_app_ctx->req_resp_ipc_connected = false;
				os_pr_info("Req/Resp client disconnected\n");
				continue;
			}
			if (ev & POLLIN) {
				handle_req_resp_command(p_app_ctx);
			}
		}
	}

	p_app_ctx->stream_ipc_connected   = false;
	p_app_ctx->req_resp_ipc_connected = false;

	os_pr_info("Shutting down IPC sockets\n");

	if (p_app_ctx->req_resp_client_sockfd >= 0) {
		(void)shutdown(p_app_ctx->req_resp_client_sockfd, SHUT_RDWR);
		(void)close(p_app_ctx->req_resp_client_sockfd);
		p_app_ctx->req_resp_client_sockfd = -1;
	}
	if (p_app_ctx->req_resp_server_sockfd >= 0) {
		(void)close(p_app_ctx->req_resp_server_sockfd);
		p_app_ctx->req_resp_server_sockfd = -1;
	}
	(void)unlink(p_app_ctx->req_resp_socket_path);

	if (p_app_ctx->stream_client_sockfd >= 0) {
		(void)shutdown(p_app_ctx->stream_client_sockfd, SHUT_RDWR);
		(void)close(p_app_ctx->stream_client_sockfd);
		p_app_ctx->stream_client_sockfd = -1;
	}
	if (p_app_ctx->stream_server_sockfd >= 0) {
		(void)close(p_app_ctx->stream_server_sockfd);
		p_app_ctx->stream_server_sockfd = -1;
	}
	(void)unlink(p_app_ctx->stream_socket_path);

	return NULL;
}


